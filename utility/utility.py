# coding: utf-8
"""
******************************************************************************
*  Copyright 2023 Labforge Inc.                                              *
*                                                                            *
* Licensed under the Apache License, Version 2.0 (the "License");            *
* you may not use this project except in compliance with the License.        *
* You may obtain a copy of the License at                                    *
*                                                                            *
*     http://www.apache.org/licenses/LICENSE-2.0                             *
*                                                                            *
* Unless required by applicable law or agreed to in writing, software        *
* distributed under the License is distributed on an "AS IS" BASIS,          *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
* See the License for the specific language governing permissions and        *
* limitations under the License.                                             *
******************************************************************************
"""
__author__ = "Thomas Reidemeister <thomas@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

import sys
import time
from PySide6.QtWidgets import QApplication, QMainWindow, QStyle, QFileDialog, QDialog, QFormLayout, QListView, \
    QDialogButtonBox, QMessageBox, QAbstractItemView, QLineEdit
from PySide6.QtGui import QIcon, QStandardItemModel, QStandardItem, QPalette
from PySide6.QtCore import QDir, QFileInfo, Qt, Signal, QThread, QFile, QIODevice
from widgets import Ui_MainWindow

# Note eBUS seems to have issues with COM initialization, import only after QApplication is initialised
eb = None
Uploader = None

LABFORGE_MAC_RANGE = '8c:1f:64:d0:e'


class BottlenoseFinderWorker(QThread):
    found = Signal(str, str)

    def find_bottlenose(self):
        system = eb.PvSystem()
        system.Find()

        # Detect, select Bottlenose.
        for i in range(system.GetInterfaceCount()):
            interface = system.GetInterface(i)
            for j in range(interface.GetDeviceCount()):
                device_info = interface.GetDeviceInfo(j)
                if device_info.GetMACAddress().GetUnicode().find(LABFORGE_MAC_RANGE) == 0:
                    self.found.emit(device_info.GetConnectionID().GetUnicode(), device_info.GetDisplayID().GetUnicode())

    def halt(self) -> None:
        self.stop = True

    def run(self):
        self.stop = False
        while not self.stop:
            self.find_bottlenose()
            time.sleep(1)


class BottlenoseSelector(QDialog):
    def __init__(self,  title, parent=None):
        super(BottlenoseSelector, self).__init__(parent=parent)
        form = QFormLayout(self)
        self.listView = QListView(self)
        form.addRow(self.listView)

        self.setWindowTitle(title)

        self.model = QStandardItemModel(self.listView)
        self.listView.setModel(self.model)
        self.listView.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.worker = BottlenoseFinderWorker()
        self.worker.found.connect(self.handle_found)
        self.worker.start()

        button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, Qt.Horizontal, self)
        form.addRow(button_box)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        self.finished.connect(self.terminate_worker)

    def terminate_worker(self, arg):
        self.worker.halt()

    def handle_found(self, addr, display):
        # Check if we have IP already
        for row in range(self.model.rowCount()):
            # BN already exists do not add twice
            item = self.model.item(row)
            if item.data(257) == addr:
                return

        standard_item = QStandardItem(display)
        standard_item.setSelectable(True)
        standard_item.setData(addr)
        self.model.appendRow(standard_item)

    def selected_bottlenose(self):
        if len(self.listView.selectedIndexes()) > 0:
            return self.listView.selectedIndexes()[0].data(Qt.UserRole + 1)
        return None


class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.ui = Ui_MainWindow()

        # Event handler registration
        self.ui.setupUi(self)

        # Event handlers
        self.ui.btnConnect.released.connect(self.handle_connect)
        self.ui.btnQuit.released.connect(self.handle_quit)
        self.ui.btnFile.released.connect(self.handle_select_file)
        self.ui.btnUpload.released.connect(self.handle_upload)

        # Style
        self.ui.cbxFileType.addItems(["Firmware", "DNN Weights"])
        self.ui.btnFile.setText("")
        self.ui.btnFile.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_FileDialogStart))
        self.ui.btnUpload.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_ArrowUp))
        self.ui.btnQuit.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_DialogCloseButton))
        self.ui.btnConnect.setIcon(QIcon.fromTheme("network-wired", QIcon(":/network-wired.png")))
        self.ui.prgUpload.setVisible(False)
        self.ui.btnUpload.setVisible(False)
        self.ui.txtFile.setReadOnly(True)

        # locals
        self.device = None
        self.uploader = None

        # Force UI into disconnected state
        self.disconnect()

    def dragEnterEvent(self, e):
        if e.mimeData().hasUrls:
            if e.mimeData().urls()[0].toLocalFile().endswith(".tar"):
                e.accept()
        else:
            e.ignore()

    def dropEvent(self, event):
        if event.mimeData().hasUrls:
            self.ui.txtFile.setText(event.mimeData().urls()[0].toLocalFile())
            self.ui.btnUpload.setVisible(True)
            self.ui.btnUpload.setEnabled(True)

    def connect_gev(self, connection_id):
        result, device = eb.PvDevice.CreateAndConnect(connection_id)
        if device is None or not result.IsOK():
            description = result.GetDescription().GetUnicode()
            if description.find("protect") >= 0:
                description = "This sensor is locked, please disconnect eBusPlayer first"
            QMessageBox.critical(self,
                                 f"Cannot connect: {result.GetCodeString().GetUnicode()}",
                                 f"Unable to connect to device: {description}")
            self.disconnect()
            return None
        else:
            self.device = device
            self.ui.txtIP.setText(device.GetIPAddress().GetUnicode())
            self.ui.txtMAC.setText(device.GetMACAddress().GetUnicode())
        return device

    def disconnect(self):
        if self.device is not None:
            self.device.Disconnect()
            self.device.Free(self.device)
            self.device = None

        # Reset network device
        self.ui.txtIP.setText("")
        self.ui.txtMAC.setText("")
        self.ui.btnFile.setEnabled(False)
        self.setAcceptDrops(False)
        self.ui.btnUpload.setEnabled(False)

    def handle_select_file(self):
        fpath = QDir.currentPath()
        if len(self.ui.txtFile.text()) > 0:
            fpath = QFileInfo(self.ui.txtFile.text()).absoluteDir().absolutePath()

        i = self.ui.cbxFileType.currentIndex()
        item = self.ui.cbxFileType.itemText(i)
        title = "Select " + item + " File"
        # if item == "Calibration":
        #     filter_text = item + " (*.json *.yaml *.yml)"
        # else:
        filter_text = item + " (*.tar)"

        selected_file, _ = QFileDialog.getOpenFileName(self, title, fpath, filter_text, "")
        if selected_file is not None and len(selected_file) > 0:
            self.ui.btnUpload.setVisible(True)
            self.ui.btnUpload.setEnabled(True)
            self.ui.txtFile.setText(selected_file)

    def handle_connect(self):
        select = BottlenoseSelector("Select Bottlenose Camera", self)
        if select.exec() == QDialog.Accepted:
            connection_id = select.selected_bottlenose()
            if connection_id is not None:
                self.disconnect()
                if self.connect_gev(connection_id) is not None:
                    self.ui.btnFile.setEnabled(True)
                    self.setAcceptDrops(True)
                    if len(self.ui.txtFile.text()) > 0:
                        self.ui.btnUpload.setVisible(True)
                        self.ui.btnUpload.setEnabled(True)

    def handle_quit(self):
        self.close()

    @staticmethod
    def validate_transfer(fname, ftype):
        if (ftype == "Firmware") or (ftype == "DNN Weights"):
            return fname.lower().endswith(".tar")
        else:
            return fname.lower().endswith(".json") or fname.lower().endswith(".yml") or fname.lower().endswith(".yaml")
        return False

    def handle_upload(self):
        fname = self.ui.txtFile.text()
        ftype = self.ui.cbxFileType.itemText(self.ui.cbxFileType.currentIndex())
        if not MainWindow.validate_transfer(fname, ftype):
            QMessageBox.warning(self, "File Error", "File Type and Name mismatch!")
            return
        if self.device is None or len(self.ui.txtIP.text()) == 0:
            QMessageBox.warning(self, "Connection Error", "Bottlenose Camera not found.")
            self.disconnect()
            return
        if not QFile.exists(fname):
            QMessageBox.warning(self, "File Error", "Could not find the specified file.")
            return

        if ftype == "Firmware":
            update_flag = "EnableUpdate"
            update_status = "UpdateStatus"
        elif ftype == "DNN Weights":
            update_flag = "EnableWeightsUpdate"
            update_status = "WeightsStatus"
        else:
            raise Exception("Unsupported Operation")

        self.uploader = Uploader(self.ui.txtIP.text(), update_flag, update_status, self.device, fname)
        self.uploader.finished.connect(self.handle_upload_finished)
        self.uploader.progress.connect(self.handle_upload_progress)
        self.uploader.error.connect(self.handle_error)
        self.ui.btnFile.setEnabled(False)
        self.ui.btnUpload.setEnabled(False)
        self.ui.btnConnect.setEnabled(False)
        self.ui.prgUpload.setVisible(True)
        self.ui.btnQuit.setEnabled(False)
        self.uploader.start()

    def handle_error(self, what):
        QMessageBox.warning(self, "Update failed", what)

    def handle_upload_progress(self, pct):
        self.ui.prgUpload.setValue(pct)

    def handle_upload_finished(self, state):
        if state:
            ftype = self.ui.cbxFileType.itemText(self.ui.cbxFileType.currentIndex())
            if ftype == "Firmware":
                QMessageBox.information(self, "Update Finished", "Please power cycle the sensor to apply the update")
            else:
                QMessageBox.information(self, "Update Finished", "Weights updated")
        self.ui.btnFile.setEnabled(True)
        self.ui.btnUpload.setEnabled(True)
        self.ui.btnConnect.setEnabled(True)
        self.ui.prgUpload.setVisible(False)
        self.ui.btnQuit.setEnabled(True)


if __name__ == '__main__':
    app = QApplication(sys.argv)

    # Instantiate import here
    import eBUS as eb
    from uploader import Uploader
    window = MainWindow()
    window.setWindowIcon(QIcon(":/labforge.ico"))
    window.show()
    sys.exit(app.exec())

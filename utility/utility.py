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
import eBUS as eb
import sys


from PySide6.QtWidgets import QApplication, QMainWindow, QStyle, QFileDialog, QDialog, QFormLayout, QListView, \
    QDialogButtonBox, QMessageBox
from PySide6.QtGui import QIcon, QStandardItemModel, QStandardItem
from PySide6.QtCore import QDir, QFileInfo, Qt
from PySide6.QtNetwork import QNetworkAccessManager
from widgets import Ui_MainWindow

LABFORGE_MAC_RANGE = '8c:1f:64:d0:e'


class BottlenoseSelector(QDialog):
    def __init__(self,  title, parent=None):
        super(BottlenoseSelector, self).__init__(parent=parent)
        form = QFormLayout(self)
        self.listView = QListView(self)
        form.addRow(self.listView)

        self.setWindowTitle(title)

        # Enumerate all bottlenose candidates
        # FIXME: move this into a background thread with polling
        model = QStandardItemModel(self.listView)
        self.find_bottlenose(model)
        self.listView.setModel(model)

        button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, Qt.Horizontal, self)
        form.addRow(button_box)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)

    def find_bottlenose(self, model):
        system = eb.PvSystem()
        system.Find()

        # Detect, select Bottlenose.
        for i in range(system.GetInterfaceCount()):
            interface = system.GetInterface(i)
            print(f"   {interface.GetDisplayID()}")
            for j in range(interface.GetDeviceCount()):
                device_info = interface.GetDeviceInfo(j)
                if device_info.GetMACAddress().GetUnicode().find(LABFORGE_MAC_RANGE) == 0:
                    standard_item = QStandardItem(f"{device_info.GetDisplayID()}")
                    standard_item.setSelectable(True)
                    standard_item.setData(device_info.GetConnectionID())
                    model.appendRow(standard_item)

    def selected_bottlenose(self):
        if len(self.listView.selectedIndexes()) > 0:
            return self.listView.selectedIndexes()[0].data(Qt.UserRole + 1)
        return None


class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.ui = Ui_MainWindow()

        self.network = QNetworkAccessManager()

        # Event handler registration
        self.ui.setupUi(self)

        # Event handlers
        self.ui.btnConnect.released.connect(self.handle_connect)
        self.ui.btnQuit.released.connect(self.handle_quit)
        self.ui.btnFile.released.connect(self.handle_select_file)
        self.network.finished.connect(self.handle_upload_finished)

        # Style
        self.ui.cbxFileType.addItems(["Firmware", "DNN Weights", "Calibration"])
        self.ui.btnFile.setText("")
        self.ui.btnFile.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_FileDialogStart))
        self.ui.btnUpload.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_ArrowUp))
        self.ui.btnQuit.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_DialogCloseButton))
        self.ui.btnConnect.setIcon(QIcon.fromTheme("network-wired", QIcon(":/network-wired.png")))
        self.ui.prgUpload.setVisible(False)
        self.ui.btnUpload.setVisible(False)

        # locals
        self.device = None
        self.status_query = None
        self.calib = None

    def __del__(self):
        self.disconnect()

    def connect_gev(self, connection_id):
        result, device = eb.PvDevice.CreateAndConnect(connection_id)
        if device is None:
            QMessageBox.critical(self,
                                 "Cannot connect",
                                 f"Unable to connect to device: {result.GetCodeString()} ({result.GetDescription()})")
            self.disconnect()
        else:
            self.device = device
            self.ui.txtIP.setText(device.GetIPAddress().GetUnicode())
            self.ui.txtMAC.setText(device.GetMACAddress().GetUnicode())
        return device


    def disconnect(self):
        if self.device is not None:
            self.device = None
            pass
        # Reset network device
        self.ui.txtIP.setText("")
        self.ui.txtMAC.setText("")

    def handle_select_file(self):
        fpath = QDir.currentPath()
        if len(self.ui.txtFile.text()) > 0:
            fpath = QFileInfo(self.ui.txtFile.text()).absoluteDir().absolutePath()

        i = self.ui.cbxFileType.currentIndex()
        item = self.ui.cbxFileType.itemText(i)
        title = "Select " + item + " File"
        if item == "Calibration":
            filter_text = item + " (*.json *.yaml *.yml)"
        else:
            filter_text = item + " (*.tar)"

        selected_file = QFileDialog.getOpenFileName(self, title, fpath, filter_text)
        if selected_file is not None and len(selected_file) > 0:
            self.ui.btnUpload.setEnabled(True)
            self.ui.txtFile.setText(selected_file)

    def handle_connect(self):
        select = BottlenoseSelector("Select Bottlenose Camera", self)
        if select.exec() == QDialog.Accepted:
            connection_id = select.selected_bottlenose()
            if connection_id is not None:
                self.disconnect()
                self.connect_gev(connection_id)
                print(connection_id)

    def handle_quit(self):
        self.close()

    def handle_upload_finished(self):
        print("Upload Finished")



if __name__ == '__main__':
    app = QApplication(sys.argv)

    window = MainWindow()
    window.setWindowIcon(QIcon(":/labforge.ico"))

    window.show()
    sys.exit(app.exec())
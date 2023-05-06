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


from PySide6.QtWidgets import QApplication, QMainWindow, QStyle, QFileDialog
from PySide6.QtGui import QIcon
from PySide6.QtCore import QDir, QFileInfo
from PySide6.QtNetwork import QNetworkAccessManager
from widgets import Ui_MainWindow


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

    def disconnect(self):
        if self.device is not None:
            # Fixme Free device
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
        print("HandleConnect")

    def handle_quit(self):
        print("HandleQuit")

    def handle_upload_finished(self):
        print("Upload Finished")



if __name__ == '__main__':
    app = QApplication(sys.argv)

    window = MainWindow()
    window.setWindowIcon(QIcon(":/labforge.ico"))

    window.show()
    sys.exit(app.exec())
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

from os import path
from argparse import ArgumentParser
import ftplib
import signal
import time
import sys
import re
from PySide6.QtCore import QThread, Signal, QCoreApplication, QFile
import eBUS as eb


class Uploader(QThread):

    error = Signal(str)
    finished = Signal(bool)
    progress = Signal(int)

    def __init__(self, address, flag_name, status_name, device, filename, parent=None):
        super().__init__(parent)
        self.address = address
        self.device = device
        self.flag = self.device.GetParameters().Get(flag_name)
        self.status = self.device.GetParameters().Get(status_name)
        self.filename = filename
        if flag_name.lower().find('weight') > 0:
            self.reset_flag = True
        else:
            self.reset_flag = False

    def run(self):
        # Sanity check to prevent firmware corruption
        if not self.reset_flag:
            res, status = self.flag.GetValue()
            if status:
                self.error.emit("Please power-cycle sensor before attempting another update.")
                self.finished.emit(False)
                return False

        self.progress.emit(0)
        if self.flag is None or self.status is None:
            self.error.emit("Function not supported by device ... please update the firmware")
            self.finished.emit(False)
            return

        if not QFile.exists(self.filename):
            self.error.emit(f"Cannot read {path.basename(self.filename)}")
            self.finished.emit(False)
            return

        self.flag.SetValue(True)
        trials = 10
        status = ""
        while not status.lower() == "ftp running" and trials > 0:
            res, status = self.status.GetValue()
            if not res.IsOK():
                self.flag.SetValue(False)
                self.error.emit(f"Unable to communicate with device: {res.GetDescription().GetUnicode()}")
                self.finished.emit(False)
                return
            time.sleep(0.1)
            trials = trials - 1

        if trials <= 0:
            self.flag.SetValue(False)
            self.error.emit(f"Unable to communicate with device: {status}")
            self.finished.emit(False)
            return

        try:
            ftp = ftplib.FTP(self.address)
            ftp.login()
            ftp.storbinary(f"STORE {path.basename(self.filename)}", fp=open(self.filename, "rb"))
            ftp.close()
        except Exception as e:
            time.sleep(0.1)

            if self.reset_flag:
                self.flag.SetValue(False)
            _, status = self.status.GetValue()
            self.error.emit(f"Unable to communicate with device: {status}")
            self.finished.emit(False)
            return

        while True:
            time.sleep(0.5)
            res, status = self.status.GetValue()
            match = re.search(r"Updating:\s+(\d+)\s*%", status)
            if match:
                number = match.group(1)
                self.progress.emit(int(number))
            elif status.find('finished') >= 0 or status.find('Loaded') >= 0:
                # Only weights update is reentrant
                if self.reset_flag:
                    self.flag.SetValue(False)

                self.progress.emit(100)
                self.finished.emit(True)
                break
            else:
                self.flag.SetValue(False)
                self.error.emit(f"Unknown error: {status}")
                self.finished.emit(False)
                break


if __name__ == '__main__':
    parser = ArgumentParser(description='Commandline Bottlenose Updater')
    parser.add_argument('--file', '-f', type=str, help='File to upload', required=True)
    parser.add_argument('--type', '-t', type=str, help='Type of file to upload {firmware, dnn}', required=True,
                        default='firmware')
    parser.add_argument('--ip', '-i', type=str, help='Address of camera to connect to', required=True)
    options = parser.parse_args()

    # Open GEV device
    result, device = eb.PvDevice.CreateAndConnect(options.ip)
    if device is None or not result.IsOK():
        description = result.GetDescription().GetUnicode()
        print(f"Could not connect to {options.ip}, cause {description}", file=sys.stderr)
        sys.exit(-1)

    qtApp = QCoreApplication(sys.argv)
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    if options.type.lower() == "firmware":
        flag = "EnableUpdate"
        status = "UpdateStatus"
    elif options.type.lower() == "dnn":
        flag = "EnableWeightsUpdate"
        status = "WeightsStatus"
    else:
        print("Invalid type specified", file=sys.stderr)
        sys.exit(-1)

    # Execute updater
    u = Uploader(options.ip, flag, status, device, options.file)
    u.finished.connect(qtApp.quit)
    u.error.connect(lambda x: print(f"Error {x}"))
    u.progress.connect(lambda x: print(f"Progress {x}"))

    u.start()
    res = qtApp.exec()

    device.Free(device)

    sys.exit(res)

/******************************************************************************
 *  Copyright 2024 Labforge Inc.                                              *
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

@file file_uploader.hpp File Uploader Header
@author Martin Tchamgoue <martin@labforge.ca>
*/
#include <QString>
#include <PvDeviceGEV.h>
#include <cstdint>
#include <QObject>
#include <QNetworkAccessManager>
#include <memory>
#include <QFile>

#ifndef _IO_FILE_UPLOADER_HPP_
#define _IO_FILE_UPLOADER_HPP_

namespace labforge::io {
class FileUploader : public QObject
{
    Q_OBJECT
public:
    FileUploader(PvDeviceGEV *device_gev,
                 QObject *parent = nullptr);
    ~FileUploader();
    void process(const QString &fid, const QString &ftype, const QString &fname);

signals:
    void error(const QString &);
    void finished(bool);
    void progress(int);
    void workingOn(const QString&);

private slots:
    void onRequestFinished(QNetworkReply *reply);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QString m_ftype;
    PvGenString *m_status;
    PvGenBoolean *m_flag;
    bool m_reset_flag;
    PvDeviceGEV *m_device;
    std::unique_ptr<QNetworkAccessManager> m_network;
    QFile m_file;

    bool setRegister(QString regname, double regvalue);
    uint32_t getNumCameras();
    void uploadCalibration(uint32_t expected_cam, const QString &fname);

    void emitError(QString err);

    bool attemptConnect(uint32_t, QString&);
    void transferFile(const QString &fname);
    void monitorTransfer();

};
}

#endif // _IO_FILE_UPLOADER_HPP_
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
#include <QFile>
#include <QFileInfo>
#include "io/file_uploader.hpp"
#include "io/calib.hpp"

#include <QRegularExpression>
#include <QtConcurrent>
#include <QNetworkReply>

using namespace labforge::io;
using namespace std;

FileUploader::FileUploader(PvDeviceGEV *device_gev, QObject *parent):QObject(parent)
{
  m_device = device_gev;
  m_network = std::make_unique<QNetworkAccessManager>(this);
  connect(m_network.get(), &QNetworkAccessManager::finished,
          this, &FileUploader::onRequestFinished);
}

FileUploader::~FileUploader()
{
  if(m_network!=nullptr) m_network.reset();
  if(m_file.isOpen()) m_file.close();
}

void FileUploader::process(const QString &fid, const QString &ftype, const QString &fname){
  if(!QFile::exists(fname)){
    emitError("Transfer file not found.");
    return;
  }

  m_reset_flag = false;
  m_ftype = ftype;

  if((fid == "FRW") || (fid == "DNN")){
    QString label = "Transfering " + ftype +" File...";
    if(fid == "FRW"){
      m_flag = dynamic_cast<PvGenBoolean *>(m_device->GetParameters()->Get("EnableUpdate"));
      m_status = dynamic_cast<PvGenString *>(m_device->GetParameters()->Get("UpdateStatus"));
      label = "Transfering Firmware File...";
    } else if(fid == "DNN"){
      m_flag = dynamic_cast<PvGenBoolean *>(m_device->GetParameters()->Get("EnableWeightsUpdate"));
      for(auto status_name:{"WeightsStatus", "DNNStatus"}){
        m_status = dynamic_cast<PvGenString *>(m_device->GetParameters()->Get(status_name));
        if(m_status != nullptr) break;
      }
      m_reset_flag = true;
      label = "Transfering DNN Weights File...";
    }

    emit workingOn(label);
    if(!attemptConnect(10, label)){
      emitError(label);
      return;
    }
    transferFile(fname);

  } else if(fid == "CLB") {
    emit workingOn("Updating Calibration...");
    uint32_t num_cameras = getNumCameras();

    QtConcurrent::run([&](uint32_t ncam, const QString &fpath){
      uploadCalibration(ncam, fpath);
    }, num_cameras, fname);

    return;

  } else {
    emitError("Unknown file transfer type.");
    return;
  }
}

void FileUploader::uploadCalibration(uint32_t expected_cam, const QString &fname){
  if(expected_cam == 0){
    emitError("Failed to access camera information.");
    return;
  }

  std::map<QString, double> kparams;
  if(!loadCalibration(fname, kparams)){
    emitError("Failed to load calibration file onto the sensor. \nThe file contents may not match the specification. \nPlease, verify and try again.");
    return;
  }
  if(kparams.size() != (expected_cam * 16)){
    emitError("The calibration file doesn't match the camera.");
    return;
  }

  uint32_t processed = 16;
  emit progress(processed);

  uint32_t step = 64 / (uint32_t)kparams.size();
  for (const auto& [kname, kvalue] : kparams){
    if(!setRegister(kname, kvalue)){
      emitError("Failed to set [" + kname + "] on the camera.");
      break;
    }

    processed += step;
    emit progress(processed);
    QThread::msleep(100);
  }

  if(setRegister("saveCalibrationData", 1)){
    emit progress(100);
    QThread::msleep(50);
    emit finished(true);
  } else {
    emitError("Failed to recalibrate the camera. \nPlease, check connection and try again.");
  }
}

bool FileUploader::attemptConnect(uint32_t trials, QString &errorMsg){
  QString status = "";
  errorMsg = "";
  bool flags = false;

  // Sanity check to prevent firmware corruption
  if(!m_reset_flag){
    bool devflag = false;
    PvString devStatus;
    PvResult res = m_flag->GetValue(devflag);
    if(!res.IsOK()){
      emitError("Failed to query camera register, please try again.");
      return false;
    }
    res = m_status->GetValue(devStatus);
    if(!res.IsOK()){
      emitError("Failed to query camera status, please try again.");
      return false;
    }
    QString status = QString(devStatus.GetAscii());

    if(devflag && (status.toLower() != "ftp running")){
      emitError("Please, power-cycle the camera before attempting another update.");
      return false;
    }
  }

  m_flag->SetValue(false);
  QThread::msleep(100);
  for(uint32_t i = 0; i < trials; ++i){
    PvResult res = m_flag->SetValue(true);
    if(res.IsOK()){
      res = m_flag->GetValue(flags);
      if(res.IsOK() && flags == true) break;
    }
    QThread::msleep(100);
  }

  if(!flags){
    m_flag->SetValue(false);
    errorMsg = "Unable to effectively communicate with device";
    return false;
  }

  for(uint32_t i = 0; i < trials; ++i){
    PvString devStatus;
    PvResult res = m_status->GetValue(devStatus);
    status = QString(devStatus.GetAscii());
    if(res.IsOK() && (status.toLower() == "ftp running")) break;
    QThread::msleep(100);
  }

  if(status.toLower() != "ftp running"){
    m_flag->SetValue(false);
    errorMsg = "Unable to communicate with device: " + status;
    return false;
  }

  return true;
}

void FileUploader::monitorTransfer(){

  if((m_flag == nullptr) || (m_status == nullptr)){
    emitError("Function not supported by device ... please update the firmware");
    return;
  }

  QRegularExpression re("Updating:\\s+(\\d+)\\s*%");
  while(true){
    QThread::msleep(500);
    PvString devStatus="";
    PvResult res = m_status->GetValue(devStatus);
    QString status(devStatus.GetAscii());
    QRegularExpressionMatch match = re.match(status);

    if (match.hasMatch()) {
      int percentage = match.captured(1).toInt();
      int processed = 50 + (int)(percentage*0.5);
      emit progress(processed);
    } else if(status.contains("finished") || status.contains("Loaded")){
        if(m_reset_flag){
          m_flag->SetValue(false);
        }
        emit progress(100);
        emit finished(true);
        break;
      } else {
        m_flag->SetValue(false);
        emitError("Unknown error: " + status);
        break;
      }
  }
}

bool FileUploader::setRegister(QString regname, double regvalue){
  PvGenParameterArray *lDeviceParams = m_device->GetParameters();
  PvGenParameter *param = lDeviceParams->Get(regname.toStdString().c_str());

  if (param == nullptr) {
    return false;
  }

  PvGenType t;
  PvResult res = param->GetType(t);
  if (!res.IsOK()) {
    return false;
  }

  switch(t){
    case PvGenTypeFloat:
      res = static_cast<PvGenFloat*>(param)->SetValue(regvalue);
      break;
    case PvGenTypeInteger:
      res = static_cast<PvGenInteger*>(param)->SetValue((int64_t)regvalue);
      break;
    case PvGenTypeCommand:
      res = static_cast<PvGenCommand*>(param)->Execute();
      break;
    default:
      return false;
  }

  return res.IsOK();
}

uint32_t FileUploader::getNumCameras(){
  PvGenParameter *reg = m_device->GetParameters()->Get("DeviceModelName");
  if(reg == nullptr) return 0;

  PvString pvmodel;
  PvResult res = static_cast<PvGenString*>(reg)->GetValue(pvmodel);

  uint32_t num_sensors = 0;
  if(res.IsOK()){
    QString model(pvmodel.GetAscii());
    num_sensors = model.endsWith("M", Qt::CaseInsensitive)?1:2;
  }

  return num_sensors;
}

void FileUploader::emitError(QString msg){
  emit error(msg);
  emit finished(false);
}

void FileUploader::transferFile(const QString &filePath){
  m_file.setFileName(filePath);
  QFileInfo fileInfo(m_file);
  if (!fileInfo.exists() || !fileInfo.isFile()){
    m_flag->SetValue(false);
    emitError("Could not find the specified file.");
    return;
  }

  if (!m_file.open(QIODevice::ReadOnly)) {
    m_flag->SetValue(false);
    emitError("Unable to access provided file.");
    return;
  }

  QString serverIP = m_device->GetIPAddress().GetAscii();
  QString serverPath = "ftp://" + serverIP + "/";
  QUrl serverURL(serverPath + fileInfo.fileName());
  serverURL.setUserName("anonymous");
  serverURL.setPassword("");
  serverURL.setPort(21);

  QNetworkReply *reply = m_network->put(QNetworkRequest(serverURL), &m_file);
  connect(reply, &QNetworkReply::uploadProgress, this, &FileUploader::onUploadProgress);
}

void FileUploader::onUploadProgress(qint64 bytesSent, qint64 bytesTotal){
  float sent = (bytesTotal != 0)?(50 * bytesSent/bytesTotal):0;
  emit progress((int)sent);
}

void FileUploader::onRequestFinished(QNetworkReply *reply){
  if (reply->error() != QNetworkReply::NoError) {
    m_flag->SetValue(false);
    emitError("Network error during file transfer [" + reply->errorString() +"]");
  }
  else{
    emit workingOn("Updating " + m_ftype + "... Do not interact with the camera.");
    QtConcurrent::run([&]{
      monitorTransfer();
   });
  }
  reply->deleteLater();
  m_file.close();
}
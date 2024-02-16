/******************************************************************************
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

@file MainWindow.hpp MainWindow class definition
@author Thomas Reidemeister <thomas@labforge.ca>
        Guy Martin Tchamgoue <martin@labforge.ca> 
*/
#ifndef __MAINWINDOW_HPP__
#define __MAINWINDOW_HPP__

#include <memory>

#include <PvDeviceInfo.h>
#include <PvPipeline.h>
#include <PvDevice.h>
#include <PvStreamGEV.h>
#include <chrono>

#include "ui_stereo_viewer.h"
#include "gev/pipeline.hpp"
#include "io/data_thread.hpp"
#include <cstdint>

class PvGenBrowserWnd;

namespace labforge::ui {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow();

public Q_SLOTS:
  void handleStart();
  void handleStop(bool fatal = false);
  void handleConnect();
  void handleDisconnect();
  void handleRecording();
  void handleStereoData(bool is_disparity);
  void handleMonoData(bool is_disparity);
  void handleError(QString msg);
  void newData(uint64_t timestamp, QImage &left, QImage &right, bool stereo=true, bool disparity=true);
  void onFolderSelect();
  void handleSave();
  void handleColormap();
  void handleFocus();
  void handleDeviceControl();

protected:
  void ShowGenWindow( PvGenBrowserWnd *aWnd, PvGenParameterArray *aArray, const QString &aTitle );
  void CloseGenWindow( PvGenBrowserWnd *aWnd );

private:
  bool connectGEV(const PvDeviceInfo *info);


  // Reflect GUI state of connected device
  void OnConnected();
  void OnDisconnected();

  Ui_MainWindow cfg;  
  std::unique_ptr<labforge::gev::Pipeline> m_pipeline;
  PvDevice * m_device;
  volatile bool m_saving;

  std::unique_ptr<labforge::io::DataThread> m_data_thread;
  PvGenBrowserWnd *m_device_browser;

  //Status counters
  uint32_t m_frameCount;
  uint32_t m_errorCount;
  std::chrono::time_point<std::chrono::system_clock> m_startTime;
  uint32_t m_payload;
  QString m_errorMsg;
  void showStatusMessage(uint32_t received=1);
  void resetStatusCounters();
};

}

#endif // #define __MAINWINDOW_HPP__
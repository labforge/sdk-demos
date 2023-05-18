#ifndef __MAINWINDOW_HPP__
#define __MAINWINDOW_HPP__

#include <memory>

#include <PvDeviceInfo.h>
#include <PvPipeline.h>
#include <PvDevice.h>
#include <PvStreamGEV.h>

#include "ui_calibrator.h"
//#include "features.hpp"
#include "gev/pipeline.hpp"
#include "io/data_thread.hpp"

namespace labforge::ui {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  virtual ~MainWindow() {};

public Q_SLOTS:
  void handleStart();
  void handleStop(bool fatal = false);
  void handleConnect();
  void handleDisconnect();
  void handleRecording();
  void handleData();
  void handleMonoData(bool is_disparity);
  void newData(QImage &left, QImage &right, bool stereo=true, bool disparity=true);
  void onFolderSelect();
  void handleSave();
  void handleColormap();
 
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
};

}

#endif // #define __MAINWINDOW_HPP__
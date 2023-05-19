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

@file calibrator.cc Calibrator application entry point
@author Thomas Reidemeister <thomas@labforge.ca>, Guy Martin Tchamgoue <martin@labforge.ca>
*/

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

#include "ui/MainWindow.hpp"
#include "gev/util.hpp"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QString>
#include <QDir>
#include <QFileDialog>
#include <QPixmap>

#include <PvDeviceGEV.h>
#include <PvStreamGEV.h>
// Note QT conflicts with AFX definitions in Pleora library, undefine QT_LIB just for this include if AFX enabled
#ifdef _AFXDLL // Windows
#undef QT_GUI_LIB
#include <Windows.h>
#include <PvDeviceFinderWnd.h>
#define QT_GUI_LIB
#else // Linux or other
#include <PvDeviceFinderWnd.h>
#endif

using namespace cv;
using namespace std;
using namespace labforge::ui;
using namespace labforge::gev;

#define MIN_MTU_REQUIRED (8500)

static int64_t s_get_mtu(PvDevice *lDevice) {
  PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
  PvGenInteger *intval = dynamic_cast<PvGenInteger *>(lDeviceParams->Get("GevSCPSPacketSize"));
  
  int64_t value = -1;
  if(intval != nullptr) {    
    intval->GetValue(value);    
  }

  return value;
}

static bool s_is_bottlenose(const PvDeviceInfo*info) {
#ifndef NDEBUG
  // In Debug mode accept any GEV device as source
  (void)info;  
  return true;
#else
  string serial(info->GetSerialNumber().GetAscii());
  std::for_each(serial.begin(), serial.end(), [](char & c){
    c = ::toupper(c);
  });
  
  //if((name.rfind("Bottlenose", 0) == 0) && (serial.rfind("8C1F64D0E", 0) == 0)) {
  if(serial.rfind("8C1F64D0E", 0) == 0) {
    return true;
  } else {
    return true;
  }
#endif // NDEBUG
}

static QImage s_yuv2_to_qimage(const cv::Mat*img) {
  Mat res;
  cvtColor(*img, res, COLOR_YUV2RGB_YUYV);
  // data pointer looses scope, deep copy needed
  return QImage((uchar*) res.data, res.cols, res.rows, res.step, QImage::Format_RGB888).copy();
}

static QImage s_mono_to_qimage(const cv::Mat*img, bool colorize=true, int colormap=COLORMAP_JET) {
  Mat res;  
  QImage::Format qformat = QImage::Format_Grayscale8;
      
  colormap = (colormap < 0)?COLORMAP_INFERNO:colormap;
  if(colorize){
    Mat img_color;
    Mat dst = img->clone();

    dst.setTo(0, dst == 65535);
    normalize(dst, res, 0, 255, NORM_MINMAX, CV_8UC1);

    applyColorMap(res, img_color, colormap);
    cv::cvtColor(img_color, res, COLOR_BGR2RGB);
    qformat = QImage::Format_RGB888;
  }
  else {
    img->convertTo(res, CV_8UC1, 0.0038910505836575876, 0);
  }
  return QImage((uchar*) res.data, res.cols, res.rows, res.step, qformat).copy();
}

static void s_load_colormap(QComboBox *cbx, int default_cm=COLORMAP_JET){
  if(cbx == nullptr) return;

  const int pixw = 256;
  const int pixh = 30;

  unsigned char xpm[pixw*pixh]; 
  Mat img_color, res;
  Mat raw_cm(pixh, pixw, CV_8UC1, xpm);
  QPixmap pixmap;
  std::tuple<int,QString> colormaps[] = {{COLORMAP_AUTUMN,"Autumn"}, {COLORMAP_BONE,"Bone"},
                                         {COLORMAP_JET,"Jet"}, {COLORMAP_WINTER,"Winter"}, 
                                         {COLORMAP_RAINBOW,"Rainbow"}, {COLORMAP_OCEAN,"Ocean"},
                                         {COLORMAP_SUMMER,"Summer"}, {COLORMAP_SPRING,"Spring"},
                                         {COLORMAP_COOL,"Cool"}, {COLORMAP_HSV,"HSV"},
                                         {COLORMAP_PINK,"Pink"}, {COLORMAP_HOT,"Hot"},
                                         {COLORMAP_PARULA,"Parula"}, {COLORMAP_MAGMA,"Magma"},
                                         {COLORMAP_INFERNO,"Inferno"}, {COLORMAP_PLASMA,"Plasma"},
                                         {COLORMAP_VIRIDIS,"Viridis"}, {COLORMAP_CIVIDIS,"Ciridis"},
                                         {COLORMAP_TWILIGHT,"Twilight"}, {COLORMAP_TWILIGHT_SHIFTED,"Twilight-Shifted"},
                                         {COLORMAP_TURBO,"Turbo"}
                                        };

  for(int i = 0; i < pixh; ++i){
    for(int k = 0; k < pixw; ++k){
      xpm[i*pixw + k] = k;
    }
  }  
  
  cbx->setIconSize(QSize(96, 16));
  for(auto colormap:colormaps){
    applyColorMap(raw_cm, img_color, get<0>(colormap));
    cv::cvtColor(img_color, res, COLOR_BGR2RGB);
    QImage qimg((uchar*)res.data, res.cols, res.rows, res.step, QImage::Format_RGB888);  
    pixmap.convertFromImage(qimg);

    QIcon icon(pixmap);    
    cbx->addItem(icon, get<1>(colormap));
    if(get<0>(colormap) == default_cm){
      cbx->setCurrentIndex(default_cm);
    }
  }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // Will apply UI file changes
  cfg.setupUi(this);
  m_saving = false;
  cfg.btnRecord->setEnabled(false); //disable recording

  cfg.btnConnect->setIcon(QIcon::fromTheme("network-wired", QIcon(":/network-wired.png")));
  QStyle *pStyle = QApplication::style();
  cfg.btnDisconnect->setIcon(pStyle->standardIcon(QStyle::SP_DialogCloseButton));
  cfg.btnFolder->setIcon(pStyle->standardIcon(QStyle::SP_DirOpenIcon));
  cfg.btnStart->setIcon(pStyle->standardIcon(QStyle::SP_MediaPlay)); 
  cfg.btnStop->setIcon(pStyle->standardIcon(QStyle::SP_MediaStop));
  cfg.btnSave->setIcon(pStyle->standardIcon(QStyle::SP_DialogSaveButton));
  cfg.btnRecord->setIcon(QIcon::fromTheme("media-record", QIcon(":/media-record.png")));
  cfg.editFolder->setText(QApplication::translate("MainWindow", QDir::currentPath().toLocal8Bit().data(), Q_NULLPTR));

  cfg.chkColormap->setVisible(false);
  cfg.labelColormap->setVisible(false);
  cfg.cbxColormap->setVisible(false);
  cfg.chkColormap->setChecked(true);  
  cfg.chkCalibrate->setVisible(true);
  cfg.chkCalibrate->setChecked(true);
  cfg.chkCalibrate->setEnabled(true);
  s_load_colormap(cfg.cbxColormap, COLORMAP_JET);

  // Register event handlers
  connect(cfg.btnStart, &QPushButton::released, this, &MainWindow::handleStart);
  // https://stackoverflow.com/questions/37639066/how-can-i-use-qt5-connect-on-a-slot-with-default-parameters
  connect(cfg.btnStop, &QPushButton::released, [this](){ handleStop(); });
  connect(cfg.btnConnect, &QPushButton::released, this, &MainWindow::handleConnect);
  connect(cfg.btnDisconnect, &QPushButton::released, this, &MainWindow::handleDisconnect);
  connect(cfg.btnFolder, &QPushButton::released, this, &MainWindow::onFolderSelect);
  connect(cfg.btnRecord, &QPushButton::released, this, &MainWindow::handleRecording);
  connect(cfg.btnSave, &QPushButton::released, this, &MainWindow::handleSave);
  connect(cfg.chkColormap, &QCheckBox::stateChanged, this, &MainWindow::handleColormap);
  
  // Force disconnected state
  OnDisconnected();

  // Notify of debug build
#ifndef NDEBUG
  QMessageBox::warning(this, "Debug Build",
                       "Debug Build: Additional Debugging Features Enabled!");
#endif
  // Check if ebus driver is installed on any interface
  if(!IsEBusLoaded()) {
    QMessageBox::warning(this, "eBus Universal Pro Driver not Loaded",
                         "EBus Universal Pro Driver is not installed!\nCamera connection might be unreliable!\n");
  }

  m_data_thread = std::make_unique<labforge::io::DataThread>();

 /* uint32_t w=3840, h=2160;
  cv::Mat points(1, 4, CV_64FC2);
    Vec2d* pptr = points.ptr<Vec2d>();
    pptr[0] = Vec2d(w/2, 0);
    pptr[1] = Vec2d(w, h/2);
    pptr[2] = Vec2d(w/2, h);
    pptr[3] = Vec2d(0, h/2);
    cv::Scalar center_mass = mean(points);
    cv::Vec2d cn(center_mass.val);
  std::cout << "Center: " << center_mass << std::endl;
  std::cout << "Center_val: " << center_mass.val << std::endl;
  std::cout << "points: " << points << std::endl;
  std::cout << "cn: " << cn << std::endl;*/
}

void MainWindow::handleStart() {
  if(m_pipeline) {
    if(m_pipeline->Start(cfg.chkCalibrate->isChecked())) {
      cfg.btnStart->setEnabled(false);
      cfg.btnStop->setEnabled(true);
      cfg.btnSave->setEnabled(true);
      cfg.btnRecord->setEnabled(true);
      
      cfg.chkCalibrate->setEnabled(false);
    }
  }
}

void MainWindow::handleStop(bool fatal) {
  if(!cfg.btnRecord->isEnabled()){
    cfg.btnSave->setEnabled(true);
    cfg.btnRecord->setEnabled(true);
    cfg.editFolder->setEnabled(true);
    cfg.btnFolder->setEnabled(true);
    return;
  }

  m_pipeline->Stop();
  cfg.btnStop->setEnabled(false);
  cfg.btnStart->setEnabled(true);
  cfg.btnSave->setEnabled(false);
  cfg.btnRecord->setEnabled(false);
  cfg.widgetLeftSensor->reset();
  cfg.widgetRightSensor->reset();
  cfg.chkCalibrate->setEnabled(true);
  
  // Check if we lost connection
  if(!m_device || !m_device->IsConnected() || fatal) {
    handleDisconnect();
  }
}

void MainWindow::OnConnected() {
  // Enable Start Button
  cfg.btnConnect->setEnabled(false);
  cfg.btnDisconnect->setEnabled(true);

  cfg.btnStart->setEnabled(true);
  cfg.btnStop->setEnabled(false);
  cfg.btnRecord->setEnabled(false);
  cfg.btnSave->setEnabled(false);

  // FIXME: Clear data canvases
}

void MainWindow::OnDisconnected() {
  // Were we streaming ?
  cfg.btnConnect->setEnabled(true);
  cfg.btnDisconnect->setEnabled(false);

  cfg.btnStart->setEnabled(false);
  cfg.btnStop->setEnabled(false);
  cfg.btnRecord->setEnabled(false);
  cfg.btnSave->setEnabled(false);

  cfg.editIP->setText(""); 
  cfg.editMAC->setText(""); 
  cfg.editModel->setText("");
  cfg.chkCalibrate->setEnabled(true);
  // FIXME: Clear data canvases
}

void MainWindow::handleRecording(){  
  cfg.btnStop->setEnabled(true);
  cfg.btnRecord->setEnabled(false);
  cfg.btnSave->setEnabled(false);
  cfg.editFolder->setEnabled(false);
  cfg.btnFolder->setEnabled(false);
  m_saving = false;

  if(!m_data_thread->setFolder(cfg.editFolder->text())){
    QMessageBox::critical(this, "Folder Error", "Could not create or find folder.");
  }

}

void MainWindow::handleSave(){  
  cfg.btnSave->setEnabled(false);
  cfg.btnRecord->setEnabled(false);
  m_saving = true;

  if(!m_data_thread->setFolder(cfg.editFolder->text())){
    QMessageBox::critical(this, "Folder Error", "Could not create or find folder.");
  }
}

void MainWindow::onFolderSelect(){    
  QString fpath = cfg.editFolder->text().isEmpty()?QDir::currentPath():cfg.editFolder->text();
  QString selected_dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                           fpath, QFileDialog::ShowDirsOnly | 
                                                           QFileDialog::DontResolveSymlinks);
  if(!selected_dir.isEmpty()){
    cfg.editFolder->setText(selected_dir);
  }
}

void MainWindow::handleConnect() {
  PvDeviceFinderWnd lFinder;
  lFinder.SetTitle( "Device Selection" );
  lFinder.SetGEVEnabled(true, true);
  lFinder.SetU3VEnabled(false, false);

// Windows AFX API needs HWND reference to parent
  cfg.btnConnect->setEnabled(false);
#ifdef _AFXDLL
  PvResult lResult = lFinder.ShowModal((HWND)winId());
  lResult = lFinder.DoEvents();
#else // Native QT library
  PvResult lResult = lFinder.ShowModal();
#endif
  cfg.btnConnect->setEnabled(true);
  const PvDeviceInfo *devinfo = lFinder.GetSelected();

  if ( !lResult.IsOK() || ( devinfo == NULL ) ) {
    OnDisconnected();
  }

  if(connectGEV(devinfo)) {
    OnConnected();    
  } else {
    OnDisconnected();
  }
  lFinder.Close();  
}

void MainWindow::handleDisconnect() {
  m_pipeline = nullptr;
  if(m_device) {
    PvDevice::Free(m_device);
    m_device = nullptr;
  }
  OnDisconnected();
}

void MainWindow::handleColormap(){
  cfg.cbxColormap->setEnabled(cfg.chkColormap->isChecked());
}

bool MainWindow::connectGEV(const PvDeviceInfo *info) {
  if(info != nullptr) {
    // Sanity check that we're talking to a Bottlenose    
    if(!s_is_bottlenose(info)) {
      
      QMessageBox::warning(this, "Unsupported Device",
                           "Selected device is not a Bottlenose Camera! ");                   
      return false;
    }
    PvResult lResult;
    PvDevice *lDevice = PvDevice::CreateAndConnect( info->GetConnectionID(), &lResult );
    if(lDevice) {     
      // Open Stream
      PvStream *lStream = PvStream::CreateAndOpen( info->GetConnectionID(), &lResult );
      if(lStream) {
        PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( lDevice );
        PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( lStream );

        cfg.editIP->setText(lDeviceGEV->GetIPAddress().GetAscii()); 
        cfg.editMAC->setText(lDeviceGEV->GetMACAddress().GetAscii()); 
        cfg.editModel->setText(info->GetModelName().GetAscii());

        bool error = false;
        if(!ConfigureStream(lDevice, lStream)) {
          QMessageBox::warning(this, "Interface Error",
                               "Could not configure stream");
          error = true;
        }
        int64_t mtu = s_get_mtu(lDevice);
        if(mtu < MIN_MTU_REQUIRED) {
          QString contents = QString("You need at least an MTU of %1 bytes on the interface.<br/>\n"
                                     "<a href=\"https://www.ibm.com/support/pages/how-do-you-change-mtu-value-linux-and-windows-operating-systems\">How to change MTU Settings on the Linux and Windows operating systems?</a>"
          ).arg(MIN_MTU_REQUIRED);
          QMessageBox::warning(this, "Interface Error",
                               contents);
          error = true;
        }

        if(error) {
          lStream->Close();
          PvStream::Free(lStream);
          PvDevice::Free(lDevice);
          return false;
        } else {
          m_device = lDevice;
          try {
            m_pipeline = make_unique<Pipeline>(lStreamGEV, dynamic_cast<PvDeviceGEV*>(m_device));
          } catch(const exception & e) {
            QMessageBox::warning(this, "Pipeline Error", e.what());
            return false;
          }
          // Connect the data reception to this
          connect(m_pipeline.get(),
                  &Pipeline::pairReceived,
                  this,
                  &MainWindow::handleData,
                  Qt::QueuedConnection);
          connect(m_pipeline.get(),
                  &Pipeline::monoReceived,
                  this,
                  &MainWindow::handleMonoData,
                  Qt::QueuedConnection);
          connect(m_pipeline.get(),
                  &Pipeline::terminated,
                  this,
                  &MainWindow::handleStop,
                  Qt::QueuedConnection);
          return true;
        }
      } else {
        QMessageBox::warning(this, "Connection Error", "Could not enable streaming.");
      }
      PvDevice::Free( lDevice );
    } else {
      QMessageBox::warning(this, "Connection Error", "Could not connect to device.");
    }
  }
  return false;
}

void MainWindow::newData(QImage &left, QImage &right, bool stereo, bool disparity) {
  // Set the image
  cfg.widgetLeftSensor->setImage(left, false);
  cfg.widgetRightSensor->setVisible(stereo);
  cfg.chkColormap->setVisible((!stereo && disparity));
  cfg.labelColormap->setVisible((!stereo && disparity));
  cfg.cbxColormap->setVisible((!stereo && disparity));

  if(stereo){      
    cfg.widgetRightSensor->setImage(right, false);
  }

  // Do we have boundingboxes or feature points
  QColor color_dnn(255, 0, 0, 255);
  QColor color_feature(0, 255, 0, 255);
 
  bool is_saving = (!cfg.btnSave->isEnabled() && m_saving);
  bool is_recording = (!cfg.btnRecord->isEnabled() && !cfg.btnSave->isEnabled() && !m_saving);
  if(is_saving || is_recording){
    m_data_thread->process(left, right);
    
    if(is_saving){
      cfg.btnSave->setEnabled(true);
      cfg.btnRecord->setEnabled(true);
      m_saving = false;
    }
  }

  // Force redraw with updated feature points / boxes
  cfg.widgetLeftSensor->redrawPixmap();
  cfg.widgetRightSensor->redrawPixmap();

  cfg.widgetLeftSensor->setStyleSheet(QStringLiteral("background-color:black; border: 2px solid green;"));
  cfg.widgetRightSensor->setStyleSheet(QStringLiteral("background-color:black; border: 2px solid green;"));

}

void MainWindow::handleData() {
  if(m_pipeline) {
    list<tuple<Mat*, Mat*>> images;
    m_pipeline->GetPairs(images);
    m_data_thread->setStereo(true);

    // Convert and display
    for (auto it = images.begin(); it != images.end(); ++it) {
      QImage q1 = s_yuv2_to_qimage(get<0>(*it));
      QImage q2 = s_yuv2_to_qimage(get<1>(*it));
      newData(q1, q2, true, false);
      delete get<0>(*it);
      delete get<1>(*it);
    }
  }
}

void MainWindow::handleMonoData(bool is_disparity){
  if(m_pipeline){
    list<tuple<Mat*, Mat*>> images;
    m_pipeline->GetPairs(images);
    m_data_thread->setStereo(false);
   
    for (auto it = images.begin(); it != images.end(); ++it) {
      QImage q1; 
      QImage q2; 

      if(is_disparity){ 
        q1 = s_mono_to_qimage(get<0>(*it), cfg.chkColormap->isChecked(), cfg.cbxColormap->currentIndex());
      }
      else{
        q1 = s_yuv2_to_qimage(get<0>(*it));
      }

      newData(q1, q2, false, is_disparity);
      delete get<0>(*it);
      delete get<1>(*it);
    }
  }
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QIcon ico(":labforge.ico");

  MainWindow w;
  a.setWindowIcon(ico);
  w.setWindowIcon(ico);

  w.show();

  return a.exec();
}

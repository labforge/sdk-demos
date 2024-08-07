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

@file viewer.cc Viewer application entry point
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
#include <PvGenBrowserWnd.h>
#define QT_GUI_LIB
#else // Linux or other
#include <PvDeviceFinderWnd.h>
#include <PvGenBrowserWnd.h>
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

static QImage s_mono_to_qimage(const cv::Mat*img, int colormap=COLORMAP_JET, int mindisp=0, int maxdisp=0) {
  Mat res;
  QImage::Format qformat = QImage::Format_Grayscale8;

  if(colormap > 0){
    Mat img_color;
    Mat dst = img->clone();

    dst.setTo(0, dst == 65535);
    if(mindisp > 0) dst.setTo(0, dst<(mindisp * 255));
    if(maxdisp > 0) dst.setTo(0, dst>(maxdisp * 255));

    normalize(dst, res, 0, 255, NORM_MINMAX, CV_8UC1);

    applyColorMap(res, img_color, colormap-1);
    cv::cvtColor(img_color, res, COLOR_BGR2RGB);
    qformat = QImage::Format_RGB888;
  }
  else {
    img->convertTo(res, CV_8UC1, 0.00392156862745098, 0);
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
  QPixmap pixmap(pixw, pixh);
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

  /* Add no color */
  pixmap.fill(QColor("white"));
  cbx->addItem(QIcon(pixmap), "Black & White");

  /* Add other color */
  for(auto colormap:colormaps){
    applyColorMap(raw_cm, img_color, get<0>(colormap));
    cv::cvtColor(img_color, res, COLOR_BGR2RGB);
    QImage qimg((uchar*)res.data, res.cols, res.rows, res.step, QImage::Format_RGB888);
    pixmap.convertFromImage(qimg);

    QIcon icon(pixmap);
    cbx->addItem(icon, get<1>(colormap));
    if(get<0>(colormap) == default_cm){
      cbx->setCurrentIndex(default_cm + 1);
    }
  }

}

static void s_load_format(QComboBox *cbx, bool isVisible=true){
  cbx->addItem("BMP (Windows Bitmap)", "BMP");
  cbx->addItem("PNG (Portable Network Graphics)", "PNG");
  cbx->addItem("JPEG (Joint Photographic Experts Group)", "JPG");
  cbx->addItem("PPM (Portable Pixmap)", "PPM");
  cbx->setCurrentIndex(0);
  cbx->setVisible(isVisible);
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

  cfg.labelColormap->setVisible(false);
  cfg.cbxColormap->setVisible(false);
  cfg.chkCalibrate->setVisible(true);
  cfg.chkCalibrate->setChecked(false);
  cfg.chkCalibrate->setEnabled(true);

  s_load_colormap(cfg.cbxColormap, COLORMAP_JET);
  s_load_format(cfg.cbxFormat, false);
  cfg.lblFormat->setVisible(false);

  cfg.lblMinDisparity->setVisible(false);
  cfg.lblMaxDisparity->setVisible(false);
  cfg.spinMinDisparity->setVisible(false);
  cfg.spinMaxDisparity->setVisible(false);

  cfg.btnDeviceControl->setEnabled(true);
  m_device_browser = new PvGenBrowserWnd();
  m_device = nullptr;

  // Register event handlers
  connect(cfg.btnStart, &QPushButton::released, this, &MainWindow::handleStart);
  // https://stackoverflow.com/questions/37639066/how-can-i-use-qt5-connect-on-a-slot-with-default-parameters
  connect(cfg.btnStop, &QPushButton::released, [this](){ handleStop(); });
  connect(cfg.btnConnect, &QPushButton::released, this, &MainWindow::handleConnect);
  connect(cfg.btnDisconnect, &QPushButton::released, this, &MainWindow::handleDisconnect);
  connect(cfg.btnFolder, &QPushButton::released, this, &MainWindow::onFolderSelect);
  connect(cfg.btnRecord, &QPushButton::released, this, &MainWindow::handleRecording);
  connect(cfg.btnSave, &QPushButton::released, this, &MainWindow::handleSave);
  connect(cfg.btnDeviceControl, &QPushButton::released, this, &MainWindow::handleDeviceControl);
  connect(cfg.cbxFocus,&QCheckBox::stateChanged, this, &MainWindow::handleFocus);
  connect(cfg.spinRuler, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setRuler);
  connect(cfg.btnUpload, &QPushButton::released, this, &MainWindow::handleUpload);
  connect(cfg.btnFile, &QPushButton::released, this, &MainWindow::onFileTransferSelect);

  //fileManager
  cfg.cbxFileType->addItem("Firmware", "FRW");
  cfg.cbxFileType->addItem("DNN Weights", "DNN");
  cfg.cbxFileType->addItem("Calibration", "CLB");

  cfg.btnFile->setText("");
  cfg.btnFile->setIcon(pStyle->standardIcon(QStyle::SP_FileDialogStart));
  cfg.btnUpload->setIcon(pStyle->standardIcon(QStyle::SP_ArrowUp));
  cfg.btnUpload->setVisible(true);
  cfg.btnUpload->setEnabled(false);
  cfg.txtFile->setReadOnly(true);
  m_upbar = nullptr;
  m_uploader = nullptr;

  // event filter
  cfg.editFolder->installEventFilter(this);
  cfg.txtFile->installEventFilter(this);

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

  //status
  resetStatusCounters();
  showStatusMessage();
}

MainWindow::~MainWindow(){
  CloseGenWindow( m_device_browser );
  if(m_device_browser){
    delete m_device_browser;
    m_device_browser = nullptr;
  }

  m_pipeline.reset();
  if(m_device) {
    PvDevice::Free(m_device);
    m_device = nullptr;
  }

  if(m_data_thread){
    m_data_thread.reset();
  }
  if(m_upbar){
    m_upbar->close();
    delete m_upbar;
  }
  if(m_uploader){
    m_uploader.reset();
  }
}

static bool validateFileType(QString fname, QString ftype);
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() == QEvent::DragEnter){
    QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
    if(e->mimeData()->hasUrls()) {
      QString fname = e->mimeData()->urls()[0].toLocalFile();

      if(obj == cfg.editFolder) {
        QFileInfo finfo(fname);
        if(finfo.exists() && finfo.isDir()){
          e->acceptProposedAction();
        } else e->ignore();
        return true;
      }

      if(obj == cfg.txtFile){
        if (validateFileType(fname, cfg.cbxFileType->currentText())){
          e->acceptProposedAction();
        } else e->ignore();
        return true;
      }
    } else e->ignore();
  }
  else if(event->type() == QEvent::Drop){
    QDropEvent *e = static_cast<QDropEvent*>(event);
    if(e->mimeData()->hasUrls()) {
      QString fname = e->mimeData()->urls()[0].toLocalFile();

      if(obj == cfg.editFolder) {
        QFileInfo finfo(fname);
        if(finfo.exists() && finfo.isDir()){
          cfg.editFolder->setText(fname);
          e->acceptProposedAction();
        } else e->ignore();
        return true;
      }

      if(obj == cfg.txtFile){
        if (validateFileType(fname, cfg.cbxFileType->currentText())){
          cfg.txtFile->setText(fname);
          e->acceptProposedAction();
        } else e->ignore();
        return true;
      }
    } else e->ignore();
  }

  return false;
}

void MainWindow::handleStart() {
  if(m_pipeline) {
    bool is_stereo = cfg.editModel->text().endsWith("_ST", Qt::CaseInsensitive);
    if(m_pipeline->Start(cfg.chkCalibrate->isChecked(), is_stereo)) {
      cfg.btnStart->setEnabled(false);
      cfg.btnStop->setEnabled(true);
      cfg.btnSave->setEnabled(true);
      cfg.btnRecord->setEnabled(true);
      cfg.btnUpload->setEnabled(false);

      cfg.chkCalibrate->setEnabled(false);
      resetStatusCounters();

      cv::Mat qMat;
      m_calib.getDepthMatrix(qMat);
      m_data_thread->setDepthMatrix(qMat);
    }
  }
}

void MainWindow::handleStop(bool fatal) {
  cfg.cbxFormat->setEnabled(true);
  m_data_thread->stop();
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
  cfg.btnUpload->setEnabled(true);

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
  cfg.btnDeviceControl->setEnabled(true);
  cfg.btnUpload->setEnabled(true);
  showStatusMessage();
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
  cfg.btnDeviceControl->setEnabled(false);
  cfg.btnUpload->setEnabled(false);
}

void MainWindow::handleRecording(){
  cfg.btnStop->setEnabled(true);
  cfg.btnRecord->setEnabled(false);
  cfg.btnSave->setEnabled(false);
  cfg.editFolder->setEnabled(false);
  cfg.btnFolder->setEnabled(false);
  cfg.cbxFormat->setEnabled(false);
  m_saving = false;

  if(!m_data_thread->setFolder(cfg.editFolder->text())){
    QMessageBox::critical(this, "Folder Error", "Could not create or find folder. Make sure you have appropriate write permission to the destination folder.");
  }

}

void MainWindow::handleSave(){
  cfg.btnSave->setEnabled(false);
  cfg.btnRecord->setEnabled(false);
  cfg.cbxFormat->setEnabled(false);
  m_saving = true;

  if(!m_data_thread->setFolder(cfg.editFolder->text())){
    QMessageBox::critical(this, "Folder Error", "Could not create or find folder. Make sure you have appropriate write permission to the destination folder.");
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
    m_calib.setParameters(m_device);
  } else {
    OnDisconnected();
  }
  lFinder.Close();
}

void MainWindow::handleDisconnect() {
  CloseGenWindow( m_device_browser );
  /* This helps clearing the wHND on windows*/
  if(m_device_browser){
    delete m_device_browser;
    m_device_browser = new PvGenBrowserWnd;
  }

  m_pipeline = nullptr;
  if(m_device) {
    PvDevice::Free(m_device);
    m_device = nullptr;
  }

  OnDisconnected();
  m_data_thread->stop();
  resetStatusCounters();
  this->statusBar()->clearMessage();
}

void MainWindow::handleFocus() {
  cfg.widgetLeftSensor->enableFocus(cfg.cbxFocus->isChecked());
  cfg.widgetRightSensor->enableFocus(cfg.cbxFocus->isChecked());
}

static bool validateFileType(QString fname, QString ftype){
  if((ftype == "Firmware") || (ftype == "DNN Weights")){
    return fname.endsWith(".tar", Qt::CaseInsensitive);
  } else if(ftype == "Calibration"){
      return (fname.endsWith(".json", Qt::CaseInsensitive) ||
              fname.endsWith(".yaml", Qt::CaseInsensitive) ||
              fname.endsWith(".yml", Qt::CaseInsensitive));
  }

  return false;
}

void MainWindow::handleUpload(){
  QString fname = cfg.txtFile->text();
  QString ftype = cfg.cbxFileType->currentText();
  QString fid = cfg.cbxFileType->currentData().toString();
  QString address = cfg.editIP->text();

  if(fname.isEmpty()){
    QMessageBox::information(this, "Empty " + ftype + " File", "Select a " + ftype + " file to upload.");
    return;
  }

  if(!validateFileType(fname, ftype)){
    QMessageBox::warning(this, "File Error", "Unrecognized " + ftype + " file type!");
    return;
  }
  if(!QFile::exists(fname)){
    QMessageBox::warning(this, "File Error", ftype + " File not found!");
    return;
  }
  if((m_device == nullptr) || (address.isEmpty())){
    QMessageBox::warning(this, "Connection Error", "Bottlenose Camera not found.");
    return;
  }

  if(m_upbar == nullptr) {
    m_upbar = new QProgressDialog("", nullptr, 0, 100, this);
    //m_upbar->setWindowFlags(m_upbar->windowFlags() | Qt::WindowCloseButtonHint); // Remove the close button
    m_upbar->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint| Qt::WindowCloseButtonHint);
    m_upbar->setWindowModality(Qt::WindowModal);
  }

  cfg.btnStart->setEnabled(false);
  cfg.btnUpload->setEnabled(false);
  cfg.btnFile->setEnabled(false);

  m_uploader = make_unique<labforge::io::FileUploader>(dynamic_cast<PvDeviceGEV*>(m_device));
  connect(m_uploader.get(),
          &labforge::io::FileUploader::error,
          this,
          &MainWindow::handleFileUploadError,
          Qt::QueuedConnection);
  connect(m_uploader.get(),
          &labforge::io::FileUploader::finished,
          this,
          &MainWindow::handleFileUploadFinished,
          Qt::QueuedConnection);
  connect(m_uploader.get(),
          SIGNAL(workingOn(const QString&)),
          m_upbar,
          SLOT(setLabelText(const QString&)),
          Qt::QueuedConnection);
  connect(m_uploader.get(), SIGNAL(progress(int)), m_upbar, SLOT(setValue(int)));
  m_uploader->process(fid, ftype, fname);

  m_upbar->exec();
}

void MainWindow::onFileTransferSelect(){
  QString fpath = cfg.txtFile->text().isEmpty()?QDir::currentPath():QFileInfo(cfg.txtFile->text()).absoluteDir().absolutePath();
  QString item = cfg.cbxFileType->currentText();
  QString title = "Select " + item + " File";
  QString filter_text = item + " (*.tar)";

  if(item == "Calibration"){
    filter_text = item + " (*.json *.yaml *.yml)";
  }
  QString selected_file = QFileDialog::getOpenFileName(this, title, fpath, filter_text);
  if(!selected_file.isEmpty()){
    cfg.txtFile->setText(selected_file);
  }
}

void MainWindow::handleFileUploadError(QString what){
  QMessageBox::warning(this, "Update failed", what);
}

void MainWindow::handleFileUploadFinished(bool success){
  if(success){
    QString ftype = cfg.cbxFileType->currentText();
    if(ftype == "Firmware")
        QMessageBox::information(this, "Update Finished", "Please, power cycle the sensor to apply the new firmware.");
    else if(ftype == "Calibration")
        QMessageBox::information(this, "Update Finished", "Calibration updated!");
    else
        QMessageBox::information(this, "Update Finished", "Weights file updated!");
  }

  cfg.btnStart->setEnabled(true);
  cfg.btnUpload->setEnabled(true);
  cfg.btnFile->setEnabled(true);

  if(m_upbar != nullptr) m_upbar->close();
  if(m_uploader != nullptr) m_uploader.reset();
}

bool isWinVisible(PvGenBrowserWnd *aWnd){
  #ifdef _AFXDLL
    PvString wTitle = aWnd->GetTitle();
    if(strcmp(wTitle.GetAscii(), "") == 0){
      return false;
    }
    HWND whandle = FindWindowA(NULL, wTitle.GetAscii());
    if(whandle == NULL) return false;
    return IsWindowVisible(whandle);
  #else
    return aWnd->GetQWidget()->isVisible();
  #endif
}

void MainWindow::ShowGenWindow( PvGenBrowserWnd *aWnd, PvGenParameterArray *aArray, const QString &aTitle )
{
  if(!aWnd) return;
  if(isWinVisible(aWnd)){
    CloseGenWindow( aWnd );
    return;
  }

  // Create, assign parameters, set title and show modeless
  aWnd->SetTitle( aTitle.toUtf8().constData() );

#ifdef _AFXDLL
  PvResult lResult = aWnd->ShowModeless((PvWindowHandle)winId());
  lResult = aWnd->DoEvents();
#else // Native QT library
  PvResult lResult = aWnd->ShowModeless( this );
#endif

  aWnd->SetGenParameterArray( aArray );
}

void MainWindow::CloseGenWindow( PvGenBrowserWnd *aWnd )
{
  if(isWinVisible(aWnd)){
    aWnd->Close();
  }
}

void MainWindow::handleDeviceControl(){
  ShowGenWindow( m_device_browser, m_device->GetParameters(), "Device Control" );
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
          //error = true;
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
                  &MainWindow::handleStereoData,
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
          connect(m_pipeline.get(),
                  &Pipeline::onError,
                  this,
                  &MainWindow::handleError,
                  Qt::QueuedConnection);
          connect(m_pipeline.get(),
                  &Pipeline::timeout,
                  this,
                  &MainWindow::handleTimeOut,
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

void MainWindow::handleTimeOut(){
  handleDisconnect();
  QMessageBox::information(this, "Connection Error", "Camera disconnected: Communication timed out.");
}

void MainWindow::newData(uint64_t timestamp, QImage &left, QImage &right, QPair<QString, QString> &label,
                         bool disparity, uint16_t *raw_disparity, int32_t min_disparity,
                         const pointcloud_t &pc) {
  // Set the image
  bool stereo = !label.second.isEmpty();
  cfg.widgetLeftSensor->setImage(left, false);
  cfg.widgetRightSensor->setVisible(stereo);
  cfg.lblDisplayRight->setVisible(stereo);

  cfg.labelColormap->setVisible(disparity);
  cfg.cbxColormap->setVisible(disparity);

  cfg.lblMinDisparity->setVisible(disparity);
  cfg.lblMaxDisparity->setVisible(disparity);
  cfg.spinMinDisparity->setVisible(disparity);
  cfg.spinMaxDisparity->setVisible(disparity);

  cfg.lblDisplayLeft->setText(label.first);
  if(stereo){
    cfg.widgetRightSensor->setImage(right, false);
    cfg.lblDisplayRight->setText(label.second);
  }

  // Do we have boundingboxes or feature points
  QColor color_dnn(255, 0, 0, 255);
  QColor color_feature(0, 255, 0, 255);

  bool is_saving = (!cfg.btnSave->isEnabled() && m_saving);
  bool is_recording = (!cfg.btnRecord->isEnabled() && !cfg.btnSave->isEnabled() && !m_saving);
  if(is_saving || is_recording){
    m_data_thread->process(timestamp, left, right, cfg.cbxFormat->currentData().toString(),
                           raw_disparity, min_disparity, pc);
    if(is_saving){
      cfg.btnSave->setEnabled(true);
      cfg.btnRecord->setEnabled(true);
      cfg.cbxFormat->setEnabled(true);
      m_saving = false;
    }
  }

  // Force redraw with updated feature points / boxes
  cfg.widgetLeftSensor->redrawPixmap();
  cfg.widgetRightSensor->redrawPixmap();

  cfg.widgetLeftSensor->setStyleSheet(QStringLiteral("background-color:black; border: 2px solid green;"));
  cfg.widgetRightSensor->setStyleSheet(QStringLiteral("background-color:black; border: 2px solid green;"));

}

void MainWindow::showStatusMessage(uint32_t rcv_images){
  auto end = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - m_startTime);
  float esecs = elapsed.count();
  float fps = (esecs > 0)? (m_frameCount/esecs): 0.00;
  float payload = (m_payload * fps)/1000000;
  QString warn = ((m_errorMsg == "AUTO_ABORTED") || (m_errorMsg == "TIMEOUT"))?"   Warning: Skipping":((m_errorMsg == "MISSING_PACKETS")?"   Last Warning: Resends":"");
  QString last_error = (m_errorCount > 0)?("   Last Error: " + m_errorMsg):"";

  QString message = "GVSP/UDP Stream: " + QString::number(rcv_images * m_frameCount) + " images" +
                    "   " + QString::number(fps, 'f', 2) + " FPS" +
                    "   " + QString::number(rcv_images * payload, 'f', 2) + " Mbps" +
                    "   Error Count: " + QString::number(m_errorCount) + last_error + warn;
  this->statusBar()->showMessage(message);
}

void MainWindow::resetStatusCounters(){
  m_frameCount = 0;
  m_errorCount = 0;
  m_payload = 0;
  m_errorMsg = "";
  m_startTime = std::chrono::system_clock::now();
}

void MainWindow::handleError(const QString &msg){
  m_errorCount += 1;
  m_errorMsg = msg;
  showStatusMessage();
}

void MainWindow::handleStereoData() {
  if(m_pipeline) {
    list<BNImageData> images;
    uint16_t *raw_disparity = nullptr;
    m_pipeline->GetPairs(images);

    m_frameCount += images.size();
    showStatusMessage(2);

    // Convert and display
    for (auto const & image:images) {
      m_payload = image.left->cols * image.left->rows * 16;
      QImage q1;
      QImage q2;
      QPair<QString, QString> label;
      bool is_disparity = false;

      if((image.left->type() == CV_16UC1) && (image.right->type() == CV_16UC1)){
        q1 = s_mono_to_qimage(image.left, cfg.cbxColormap->currentIndex(), cfg.spinMinDisparity->value(), cfg.spinMaxDisparity->value());
        raw_disparity = (uint16_t*)image.left->data;
        q2 = s_mono_to_qimage(image.right, cfg.cbxColormap->currentIndex(), cfg.spinMinDisparity->value(), cfg.spinMaxDisparity->value());

        label.first = "Disparity";
        label.second = "Confidence";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_DC);
        is_disparity = true;
      } else if((image.left->type() == CV_8UC2) && (image.right->type() == CV_16UC1)){
        q1 = s_yuv2_to_qimage(image.left);
        q2 = s_mono_to_qimage(image.right, cfg.cbxColormap->currentIndex(), cfg.spinMinDisparity->value(), cfg.spinMaxDisparity->value());
        raw_disparity = (uint16_t*)image.right->data;
        label.first = "Left";
        label.second = "Disparity";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_LD);
        is_disparity = true;
      } else if((image.left->type() == CV_16UC1) && (image.right->type() == CV_8UC2)){
        q1 = s_mono_to_qimage(image.left, cfg.cbxColormap->currentIndex(), cfg.spinMinDisparity->value(), cfg.spinMaxDisparity->value());
        q2 = s_yuv2_to_qimage(image.right);
        raw_disparity = (uint16_t*)image.left->data;
        label.first = "Disparity";
        label.second = "Right";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_DR);
        is_disparity = true;
      } else{
        q1 = s_yuv2_to_qimage(image.left);
        q2 = s_yuv2_to_qimage(image.right);
        label.first = "Left";
        label.second = "Right";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_LR);
      }

      newData(image.timestamp, q1, q2, label, is_disparity, raw_disparity, image.min_disparity, image.pc);
      delete image.left;
      delete image.right;
    }
  }
}

void MainWindow::handleMonoData(){
  if(m_pipeline){
    list<BNImageData> images;
    uint16_t *raw_disparity = nullptr;
    m_pipeline->GetPairs(images);

    m_frameCount += images.size();
    showStatusMessage(1);

    for (auto const& image:images) {
      m_payload = image.left->cols * image.left->rows * 16;
      QImage q1;
      QImage q2;
      QPair<QString, QString> label;
      bool is_disparity = false;

      if(image.left->type() == CV_16UC1){
        q1 = s_mono_to_qimage(image.left, cfg.cbxColormap->currentIndex(), cfg.spinMinDisparity->value(), cfg.spinMaxDisparity->value());
        raw_disparity = (uint16_t*)image.left->data;
        label.first = "Disparity";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_DO);
        is_disparity = true;
      }
      else{
        q1 = s_yuv2_to_qimage(image.left);
        label.first = "Display";
        m_data_thread->setImageDataType(labforge::io::IMTYPE_IO);
      }
      label.second = "";

      newData(image.timestamp, q1, q2, label, is_disparity, raw_disparity, image.min_disparity, image.pc);
      delete image.left;
      delete image.right;
    }
  }
}

void MainWindow::setRuler(int value) {
  cfg.widgetLeftSensor->setRuler(value);
  cfg.widgetRightSensor->setRuler(value);
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

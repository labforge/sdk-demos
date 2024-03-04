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

@file data_thread.cc DataThread implementation
@author Guy Martin Tchamgoue <martin@labforge.ca>, Thomas Reidemeister <thomas@labforge.ca>
*/
#include <QDir>
#include "io/data_thread.hpp"

#include <algorithm>
#include <fstream>
#include <cmath>

using namespace labforge::io;
using namespace std;

DataThread::DataThread(QObject *parent)
    : QThread(parent), m_abort(false)
{
  m_folder = "";
  m_left_subfolder = "cam0";
  m_right_subfolder = "cam1";  
  m_disparity_subfolder = "disparity";  
  m_frame_counter = 0;        
  m_stereo = true;
  m_disparity = false;
}

DataThread::~DataThread()
{
  m_mutex.lock();
  m_abort = true;    
  m_queue.clear();
  m_mutex.unlock();
  m_condition.wakeOne();
  wait();
}

void DataThread::process(uint64_t timestamp, const QImage &left_image, const QImage &right_image, QString format, const uint16_t *raw){
  QMutexLocker locker(&m_mutex);
  
  cv::Mat dmat;
  if(raw != nullptr){
    dmat = cv::Mat(left_image.height(),left_image.width(), CV_16UC1, (uint16_t *)raw);    
  }
  m_queue.enqueue({timestamp, left_image, right_image, format, dmat});
  
  if (!isRunning()) {
    start(HighPriority);
  } else {        
    m_condition.wakeOne();
  }
}

bool getFilename(QString &fname, const QString &new_folder, const QString &subfolder, QString file_prefix){
  QDir qdir(new_folder);  
  QString subdir_path = qdir.filePath(subfolder);
  if(!qdir.mkpath(subdir_path)){     
    return false;
  }
  
  qdir.cd(subfolder);    
  fname = qdir.absoluteFilePath(file_prefix); 
  return true;
}

bool DataThread::setFolder(QString new_folder){
  bool status = true;
  QMutexLocker locker(&m_mutex);

  if(new_folder != m_folder){
    m_folder = new_folder;
    m_frame_counter = 0;
  }  

  if(m_stereo){
    if(m_disparity){
      status = getFilename(m_left_fname, m_folder, m_left_subfolder, "left_");
      status = status && getFilename(m_disparity_fname, m_folder, m_disparity_subfolder, "disparity_");
    }else{
      status = getFilename(m_left_fname, m_folder, m_left_subfolder, "left_");
      status = status && getFilename(m_right_fname, m_folder, m_right_subfolder, "right_");
    }
  }else{
    if(m_disparity){
      status = getFilename(m_disparity_fname, m_folder, m_disparity_subfolder, "disparity_");
    } else {
      status = getFilename(m_left_fname, m_folder, m_left_subfolder, "mono_");
    }
  }
  
  return status;
}

void DataThread::setStereoDisparity(bool is_stereo, bool is_disparity){
  m_stereo = is_stereo;
  m_disparity = is_disparity;
}

void DataThread::stop(){
  QMutexLocker locker(&m_mutex);
  m_queue.clear();
  m_condition.wakeOne();
}  

static inline bool invalid(cv::Point3f &pt){
  return (std::isnan(pt.x) || std::isnan(pt.y) || std::isnan(pt.z) ||
          std::isinf(pt.x) || std::isinf(pt.y) || std::isinf(pt.z)); 
}

static uint32_t countNaN(const cv::Mat& pointCloud){
  uint32_t count = 0;
  for (int y = 0; y < pointCloud.rows; ++y) {
      for (int x = 0; x < pointCloud.cols; ++x) {
        cv::Point3f pt = pointCloud.at<cv::Point3f>(y, x);
        if(invalid(pt)) count++;          
      }
  }
  return count;
}

static void saveColoredPLYFile(const cv::Mat& pointCloud, QImage &image, const QString& filename) {
  ofstream plyFile(filename.toStdString());

  uint32_t nan_counts = countNaN(pointCloud);
  uint32_t pc_size = pointCloud.rows * pointCloud.cols - nan_counts;

  // Write PLY header
  plyFile << "ply\n";
  plyFile << "format ascii 1.0\n";
  plyFile << "element vertex " << pc_size << "\n";
  plyFile << "property float x\n";
  plyFile << "property float y\n";
  plyFile << "property float z\n";
  plyFile << "property uchar red\n";
  plyFile << "property uchar green\n";
  plyFile << "property uchar blue\n";
  plyFile << "end_header\n";
  
  // Write point cloud data
  for (int y = 0; y < pointCloud.rows; ++y) {
      for (int x = 0; x < pointCloud.cols; ++x) {
          cv::Point3f pt = pointCloud.at<cv::Point3f>(y, x);
          if(invalid(pt)) continue;

          QColor color = image.pixelColor(x, y);
          
          plyFile << pt.x << " " << pt.y << " " << pt.z << " "
                  << color.red() << " "  // Red
                  << color.green() << " "  // Green
                  << color.blue() << "\n"; // Blue
      }
  }

  plyFile.close();
}

static void project3D(cv::Mat &dispf32, cv::Mat &xyz, cv::Mat &qmat){
  
  cv::Mat_<float> vec_tmp(4,1);
  for(int y=0; y < dispf32.rows; ++y) {
      for(int x=0; x < dispf32.cols; ++x) {
          vec_tmp(0) = x; 
          vec_tmp(1) = y; 
          vec_tmp(2) = dispf32.at<float>(y,x); 
          vec_tmp(3) = 1;

          vec_tmp = qmat * vec_tmp;
          vec_tmp /= vec_tmp(3);

          cv::Vec3f &point = xyz.at<cv::Vec3f>(y,x);
          point[0] = vec_tmp(0);
          point[1] = vec_tmp(1);
          point[2] = vec_tmp(2);
          if(vec_tmp(2) > 7) point[2] = 0;
      }
  }
}

void DataThread::run() {    
  while(!m_abort) {
    m_mutex.lock();
    while(m_queue.isEmpty() && !m_abort){
      m_condition.wait(&m_mutex);
    }
    if(m_abort){
      m_mutex.unlock();
      break;
    }
    ImageData imdata = m_queue.dequeue();
    m_mutex.unlock();

    QString ext = imdata.format.toUpper();        
    QString padded_cntr = QString("%1").arg(m_frame_counter, 4, 10, QChar('0')); 
    QString suffix =  padded_cntr + "_" + QString::number(imdata.timestamp)  + "." + ext.toLower();                
    int32_t quality = (ext == "JPG") ? 90 : -1;

    if (m_stereo){
      if(m_disparity){            
        imdata.left.save(m_left_fname + suffix, ext.toStdString().c_str(), quality);         
        imdata.right.save(m_disparity_fname + suffix, ext.toStdString().c_str(), quality);  
        
        if(!imdata.disparity.empty()){
          //float Q[16] = {1.0,0.0,0.0,-1000.52124,0.0,1.0,0.0,-584.05273,0.0,0.0,0.0,3923.664,0.0,0.0,7.4457912,96.79574};
          //float Q[16] = {1.0,0.0,0.0,-890.5212,0.0,1.0,0.0,-594.0528,0.0,0.0,0.0,3923.6646,0.0,0.0,7.4457903,-0.0};
          //float Q[16] = {1.0,0.0,0.0,-898.6985,0.0,1.0,0.0,-592.93506,0.0,0.0,0.0,3893.555,0.0,0.0,7.4382367,-0.0};
          //cv::Mat qmat(4, 4, CV_32F, Q);

          cv::Mat pc(imdata.left.height(), imdata.left.width(), CV_32FC3);      
          cv::Mat dispf32;

          //imdata.disparity.setTo(0, imdata.disparity == 65535);
          imdata.disparity.convertTo(dispf32, CV_32FC1, (1./255.0), 0);
          /*cv::Mat filtered;
          cv::medianBlur(imdata.disparity, filtered, 3);
          filtered.convertTo(dispf32, CV_32FC1, (1./255.0), 0);*/

          QString fname = m_disparity_fname + suffix.replace(ext.toLower(), "ply");
          if(!m_matQ.empty()){               
            cv::reprojectImageTo3D(dispf32, pc, m_matQ, false, CV_32F);          
            saveColoredPLYFile(pc, imdata.left, fname);
          }
        }
      } else {       
        imdata.left.save(m_left_fname + suffix, ext.toStdString().c_str(), quality);            
        imdata.right.save(m_right_fname + suffix, ext.toStdString().c_str(), quality);
      }
    } else {
      if(m_disparity){
        imdata.left.save(m_disparity_fname + suffix, ext.toStdString().c_str(), quality);
      } else {
        imdata.left.save(m_left_fname + suffix, ext.toStdString().c_str(), quality);
      }
    }        
    
    m_frame_counter += 1;               
  }
}

void  DataThread::setDepthMatrix(cv::Mat& qmat){
  m_matQ = qmat.clone();
  std::cout << "m_matQ = " << std::endl << " "  << m_matQ << std::endl << std::endl;
}
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

#define MAX_QUEUE_SIZE 5

using namespace labforge::io;
using namespace std;

DataThread::DataThread(QObject *parent)
    : QThread(parent), m_abort(false )
{
    m_folder = "";
    m_left_subfolder = "cam0";
    m_right_subfolder = "cam1";  
    m_disparity_subfolder = "disparity";  
    m_frame_counter = 0;        
    m_stereo = true;
}

DataThread::~DataThread()
{
    m_mutex.lock();
    m_abort = true;
    m_condition.wakeOne();
    m_mutex.unlock();

    wait();
}

void DataThread::process(const QImage &left_image, const QImage &right_image){
    QMutexLocker locker(&m_mutex);
    //-check queue size  
    if(m_queue.size() < MAX_QUEUE_SIZE){
        m_queue.enqueue({left_image, right_image});
    }

    if (!isRunning()) {
        start(LowPriority);
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
  QMutexLocker locker(&m_mutex);

  if(new_folder != m_folder){
    m_folder = new_folder;
    m_frame_counter = 0;
  }  
  
  if(!getFilename(m_left_fname, m_folder, m_left_subfolder, "left_")){
    return false;
  } 
  if(!getFilename(m_right_fname, m_folder, m_right_subfolder, "right_")){
    return false;
  } 
  if(!getFilename(m_disparity_fname, m_folder, m_disparity_subfolder, "disparity_")){
    return false;
  }  

  return true;
}

void DataThread::run() {    
    while(!m_abort) {
        m_mutex.lock();
        while(m_queue.isEmpty() && !m_abort){
           m_condition.wait(&m_mutex);
        }
        if(m_abort) return;

        ImageData imdata = m_queue.dequeue();
        m_mutex.unlock();
                
        QString suffix = QString::number(m_frame_counter)  + ".png";                
        if(m_stereo){
            imdata.left.save(m_left_fname + suffix, "PNG");
            imdata.right.save(m_right_fname + suffix, "PNG");           
        }else{
            imdata.left.save(m_disparity_fname + suffix, "PNG");
        }
        m_frame_counter += 1;               
    }
}

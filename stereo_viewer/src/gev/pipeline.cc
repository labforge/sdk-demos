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

@file pipeline.cc Pipeline implementation
@author Thomas Reidemeister <thomas@labforge.ca>
        Guy Martin Tchamgoue <martin@labforge.ca>
*/
#include "gev/pipeline.hpp"
#include "gev/util.hpp"
#include <stdexcept>
#include <iostream>

using namespace labforge::gev;
using namespace std;
using namespace cv;

#define MAX_CONS_ERRORS_IN_ACQUISITION 5

Pipeline::Pipeline(PvStreamGEV *stream_gev, PvDeviceGEV *device_gev, QObject * parent) : QThread(parent) {
  m_stream = stream_gev;
  m_device = device_gev;

  // Enable Multipart
  if(!SetParameter(m_device, m_stream, "GevSCCFGMultiPartEnabled", true)) {
    throw runtime_error("Could not set multipart for stereo transfer");
  }
  CreateStreamBuffers(m_device, m_stream, &m_buffers, 16);
  if(m_buffers.empty()) {
    throw runtime_error("Could allocate stream buffers");
  }
 
  // Map start and stop and status commands
  PvGenParameterArray *lDeviceParams = m_device->GetParameters();
  m_start = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
  m_stop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );
  m_pixformat = dynamic_cast<PvGenEnum *>( lDeviceParams->Get( "PixelFormat" ) );
  m_rectify  = dynamic_cast<PvGenBoolean *>( lDeviceParams->Get( "Rectification" ) );
  m_undistort = dynamic_cast<PvGenBoolean *>( lDeviceParams->Get( "Undistortion" ) );
  // Get stream parameters
  PvGenParameterArray *lStreamParams = m_stream->GetParameters();
  m_fps = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRate" ) );
  m_bandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "Bandwidth" ) );

  if(!m_start ||
     !m_stop) {
    throw runtime_error("Could map stream start and stop commands");
  }
  if(!m_fps ||
     !m_bandwidth ||
     !m_pixformat ||      
     !m_undistort){
      throw runtime_error("Unable to initialise critical camera features. Please, make sure the camera is accessible.");
     }

  if(m_rectify) m_rectify->GetValue(m_rectify_init);  
  m_undistort->GetValue(m_undistort_init);
  m_pixformat->GetValue( m_pixfmt_init );    

  m_start_flag = false;
}

Pipeline::~Pipeline() {
  // Terminate worker if active
  if(m_start_flag){
    Stop();
  }
  else{
    if(m_rectify) m_rectify->SetValue(m_rectify_init);
    m_undistort->SetValue(m_undistort_init);
    m_pixformat->SetValue(m_pixfmt_init);
  }

  // Close stream when pipeline is destroyed
  if(m_stream != nullptr) {
    m_stream->Close();
    PvStream::Free(m_stream);
  }
  if(!m_buffers.empty()) {
    FreeStreamBuffers(&m_buffers);
  }
}

bool Pipeline::Start(bool calibrate) {  
  // Queue all buffers
  auto lIt = m_buffers.begin();
  while ( lIt != m_buffers.end() )
  {
    m_stream->QueueBuffer( *lIt );
    lIt++;
  }
  
  bool rct_value = calibrate?!calibrate:m_rectify_init;
  bool und_value = calibrate?!calibrate:m_undistort_init;
  PvString pixformat = calibrate?"YUV422_8":m_pixfmt_init.GetAscii();
          
  if(m_rectify) m_rectify->SetValue(rct_value);
  m_undistort->SetValue(und_value);    
  m_pixformat->SetValue(pixformat);
  
  PvResult res = m_device->StreamEnable();
  if(!res.IsOK())
    return false;
  res = m_start->Execute();
  if(!res.IsOK())
    return false;
  m_start_flag = true;
  start(); // Start thread
  return true;
}

size_t Pipeline::GetPairs(list<tuple<Mat *, Mat *, uint64_t>> &out) {
  QMutexLocker l(&m_image_lock);
  size_t res = m_images.size();
  for (auto it = m_images.begin(); it != m_images.end(); ++it) {
    out.push_back(*it);
  }
  m_images.clear();

  return res;
}

void Pipeline::Stop() {
  m_start_flag = false;
  // Wait for thread to terminate
  wait();
  // Discard all queued buffers

  if(m_rectify) m_rectify->SetValue(m_rectify_init);
  m_undistort->SetValue(m_undistort_init);    
  m_pixformat->SetValue(m_pixfmt_init);
  QThread::currentThread()->usleep(100*1000);  
}

void Pipeline::run() {
  size_t consequitive_errors = 0;  
  
  while(m_start_flag) {
    PvBuffer *lBuffer = nullptr;
    PvResult lOperationResult;
    PvString pixformat;
    bool is_disparity = true;
    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;
    uint64_t timestamp;

    // Retrieve next buffer
    PvResult lResult = m_stream->RetrieveBuffer( &lBuffer, &lOperationResult, 1500 );
    if (lResult.IsOK()) {
      if (lOperationResult.IsOK()) {
        //
        // We now have a valid buffer. This is where you would typically process the buffer.
        // -----------------------------------------------------------------------------------------
        // ...
        consequitive_errors = 0;

        m_fps->GetValue( lFrameRateVal );
        m_bandwidth->GetValue( lBandwidthVal );
        timestamp = lBuffer-> GetTimestamp();

        IPvImage *img0, *img1;
        switch ( lBuffer->GetPayloadType() ) {
          case PvPayloadTypeMultiPart:
            img0 = lBuffer->GetMultiPartContainer()->GetPart(0)->GetImage();
            img1 = lBuffer->GetMultiPartContainer()->GetPart(1)->GetImage();
            m_pixformat->GetValue( pixformat );
            
            // Protected image creation
            {
              int cv_pixfmt0 = (img0->GetPixelType() == PvPixelYUV422_8)? CV_8UC2: CV_16UC1;
              int cv_pixfmt1 = (img1->GetPixelType() == PvPixelYUV422_8)? CV_8UC2: CV_16UC1;
              is_disparity = ((cv_pixfmt0 == CV_16UC1) || (cv_pixfmt1 == CV_16UC1));

              QMutexLocker l(&m_image_lock);
              // See if there is chunk data attached
              
              m_images.push_back(
                      make_tuple(
                              new Mat(img0->GetHeight(), img0->GetWidth(), cv_pixfmt0, img0->GetDataPointer()),
                              new Mat(img1->GetHeight(), img1->GetWidth(), cv_pixfmt1, img1->GetDataPointer()),
                              timestamp
                              )
                              );
            }
            
            emit pairReceived(is_disparity);
            break;

          case PvPayloadTypeImage:                        
            {
              QMutexLocker l(&m_image_lock);
              img0 = lBuffer->GetImage();
              m_pixformat->GetValue( pixformat );
              int cv_pixformat = CV_16UC1;
              is_disparity = true;

              if(strcmp(pixformat.GetAscii(), "YUV422_8") == 0){
                cv_pixformat = CV_8UC2;
                is_disparity = false;
              }

              m_images.push_back( make_tuple(new Mat(img0->GetHeight(), img0->GetWidth(), cv_pixformat, img0->GetDataPointer()), 
                                             new Mat(), timestamp));
            }
            
            emit monoReceived(is_disparity);
            break;

          default:
            // Invalid buffer received
            
            cout << "FMT_ERR(" << consequitive_errors << ") :" << lResult.GetCodeString().GetAscii() << endl;
            consequitive_errors++;
            break;
        }
      } else {
        // Non OK operational result, wait 100ms before retry
        consequitive_errors++;
        cout << "OP_ERR(" << consequitive_errors << ") :" << lOperationResult.GetCodeString().GetAscii() << endl;
        QThread::currentThread()->usleep(100*1000);
      }
      // Re-queue the buffer in the stream object
      m_stream->QueueBuffer( lBuffer );
    }
    else {
      // Retrieve buffer failure, wait 100ms before retry
      QThread::currentThread()->usleep(100*1000);
      consequitive_errors++;
      cout << "BUF_ERR(" << consequitive_errors << ") :" << lResult.GetCodeString().GetAscii() << endl;
    }
    if(consequitive_errors > MAX_CONS_ERRORS_IN_ACQUISITION) {
      m_start_flag = false;
    }
  }

  // Tell the device to stop sending images.
  m_stop->Execute();

  // Disable streaming on the device
  m_device->StreamDisable();

  // Abort all buffers from the stream and dequeue
  m_stream->AbortQueuedBuffers();
  while ( m_stream->GetQueuedBufferCount() > 0 ) {
    PvBuffer *lBuffer = nullptr;
    PvResult lOperationResult;

    m_stream->RetrieveBuffer( &lBuffer, &lOperationResult );
  }

  // Discard retrieved pairs
  {
    QMutexLocker l(&m_image_lock);
    for (auto it = m_images.begin(); it != m_images.end(); ++it) {
      delete get<0>(*it);
      delete get<1>(*it);
    }
    
    m_images.clear();
  }

  // Mark terminated
  emit terminated(consequitive_errors > MAX_CONS_ERRORS_IN_ACQUISITION);
}

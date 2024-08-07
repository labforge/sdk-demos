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
#include <vector>
#include <bottlenose_chunk_parser.hpp>

using namespace labforge::gev;
using namespace std;
using namespace cv;

#define MAX_CONS_ERRORS_IN_ACQUISITION 5

Pipeline::Pipeline(PvStreamGEV *stream_gev, PvDeviceGEV *device_gev, QObject * parent) : QThread(parent) {
  m_stream = stream_gev;
  m_device = device_gev;
  PvResult res;

  // Enable Multipart
  if(!SetParameter(m_device, m_stream, "GevSCCFGMultiPartEnabled", true)) {
    throw runtime_error("Could not set multipart for stereo transfer");
  }
  // Enable meta information chunk for reliable timestamping
  if(!SetParameter(m_device, m_stream, "ChunkModeActive", true)) {
    throw runtime_error("Could not enable chunk data transfer");
  }
  // Select the appropriate enumerator for chunk
  PvGenParameterArray *lDeviceParams = m_device->GetParameters();
  PvGenParameter *param = lDeviceParams->Get("ChunkSelector");
  if (param == nullptr) {
    throw runtime_error("Could not enable access chunk selector");
  }
  PvGenType t;
  res = param->GetType(t);
  if (!res.IsOK()) {
    throw runtime_error("Could not enable access chunk selector");
  }
  if(t == PvGenTypeEnum){
    res = static_cast<PvGenEnum *>( param )->SetValue( "FrameInformation" );
    if(!res.IsOK()) {
      throw runtime_error("Could not select frame information chunk");
    }
  }
  if(!SetParameter(m_device, m_stream, "ChunkEnable", true)) {
    throw runtime_error("Could not enable frame information chunk");
  }
  CreateStreamBuffers(m_device, m_stream, &m_buffers, 16);
  if(m_buffers.empty()) {
    throw runtime_error("Could allocate stream buffers");
  }

  // Map start and stop and status commands
  m_start = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
  m_stop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );
  m_mindisparity = dynamic_cast<PvGenInteger *>( lDeviceParams->Get( "MinimumDisparity" ) );

  // Get stream parameters
  PvGenParameterArray *lStreamParams = m_stream->GetParameters();
  m_fps = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRate" ) );
  m_bandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "Bandwidth" ) );

  if(!m_start || !m_stop) {
    throw runtime_error("Could map stream start and stop commands");
  }
  if(!m_fps || !m_bandwidth){
    throw runtime_error("Unable to initialise critical camera features. Please, make sure the camera is accessible.");
  }

  m_start_flag = false;
}

Pipeline::~Pipeline() {
  // Terminate worker if active
  if(m_start_flag){
    Stop();
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

void Pipeline::enterCalibrationMode(bool enable, bool stereo){
  if(!enable) return;

  if(!SetParameter(m_device, m_stream, "Undistortion", false)) {
    throw runtime_error("Could not disable Undistortion");
  }

  if(!stereo) return;

  PvGenParameterArray *lDeviceParams = m_device->GetParameters();
  PvGenParameter *param = lDeviceParams->Get("ComponentSelector");

  if (param == nullptr) {
    if(!SetParameter(m_device, m_stream, "PixelFormat", "YUV422_8")) {
      throw runtime_error("Could not enable PixelFormat");
      return;
    }
    if(!SetParameter(m_device, m_stream, "Rectification", false)) {
      throw runtime_error("Could not disable Rectification");
      return;
    }
    return;
  }

  PvResult res = static_cast<PvGenEnum *>(param)->SetValue("Confidence");
  if(!res.IsOK()) {
    throw runtime_error("Could not select Component value [Confidence]");
  }
  if(!SetParameter(m_device, m_stream, "ComponentEnable", false)) {
    throw runtime_error("Could not disable Component Confidence");
  }
  res = static_cast<PvGenEnum *>(param)->SetValue("IntensityLeft");
  if(!res.IsOK()) {
    throw runtime_error("Could not select Component value [IntensityLeft]");
  }
  if(!SetParameter(m_device, m_stream, "ComponentEnable", true)) {
    throw runtime_error("Could not enable Component IntensityLeft");
  }
  res = static_cast<PvGenEnum *>(param)->SetValue("Disparity");
  if(!res.IsOK()) {
    throw runtime_error("Could not select Component value [Disparity]");
  }
  if(!SetParameter(m_device, m_stream, "ComponentEnable", false)) {
    throw runtime_error("Could not disable Component Disparity");
  }
  res = static_cast<PvGenEnum *>(param)->SetValue("IntensityRight");
  if(!res.IsOK()) {
    throw runtime_error("Could not select Component value [IntensityRight]");
  }
  if(!SetParameter(m_device, m_stream, "ComponentEnable", true)) {
    throw runtime_error("Could not enable Component IntensityRight");
  }
  res = static_cast<PvGenEnum *>(param)->SetValue("IntensityLeft");

  if(!SetParameter(m_device, m_stream, "Rectification", false)) {
    throw runtime_error("Could not disable Rectification");
  }
}

bool Pipeline::Start(bool calibrate, bool is_stereo) {
  // Queue all buffers
  auto lIt = m_buffers.begin();
  while ( lIt != m_buffers.end() )
  {
    m_stream->QueueBuffer( *lIt );
    lIt++;
  }

  enterCalibrationMode(calibrate, is_stereo);

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

size_t Pipeline::GetPairs(list<BNImageData> &out) {
  QMutexLocker l(&m_image_lock);

  if(!m_images.empty()){
    BNImageData image = m_images.dequeue();
    out.push_back(image);
  }

  return 0;
}

void Pipeline::Stop() {
  m_start_flag = false;
  // Wait for thread to terminate
  wait();
  // Discard all queued buffers
}

void Pipeline::run() {
  size_t consequitive_errors = 0;
  size_t timeout_count = MAX_CONS_ERRORS_IN_ACQUISITION;

  while(m_start_flag) {
    PvBuffer *lBuffer = nullptr;
    PvResult lOperationResult;

    double lFrameRateVal = 0.0;
    double lBandwidthVal = 0.0;
    uint64_t timestamp;
    int64_t minDisparity = 0;
    info_t info = {};
    pointcloud_t pointcloud;

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
        if(chunkDecodeMetaInformation(lBuffer, &info)) {
          std::cout << "Bottlenose time: " << ms_to_date_string(info.real_time) << endl;
          timestamp = info.real_time;
        } else {
          cerr << "Could not decode meta information" << endl;
        }

        chunkDecodePointCloud(lBuffer, pointcloud);

        if(m_mindisparity){
          m_mindisparity->GetValue(minDisparity);
        }

        IPvImage *img0, *img1;
        switch ( lBuffer->GetPayloadType() ) {
          case PvPayloadTypeMultiPart:
            img0 = lBuffer->GetMultiPartContainer()->GetPart(0)->GetImage();
            img1 = lBuffer->GetMultiPartContainer()->GetPart(1)->GetImage();

            // Protected image creation
            {
              int cv_pixfmt0 = (img0->GetPixelType() == PvPixelYUV422_8)? CV_8UC2: CV_16UC1;
              int cv_pixfmt1 = (img1->GetPixelType() == PvPixelYUV422_8)? CV_8UC2: CV_16UC1;

              QMutexLocker l(&m_image_lock);
              // See if there is chunk data attached

              m_images.enqueue({
                              new Mat(img0->GetHeight(), img0->GetWidth(), cv_pixfmt0, img0->GetDataPointer()),
                              new Mat(img1->GetHeight(), img1->GetWidth(), cv_pixfmt1, img1->GetDataPointer()),
                              timestamp, static_cast<int32_t>(minDisparity), pointcloud
                              }
                              );
            }

            emit pairReceived();
            break;

          case PvPayloadTypeImage:
            {
              QMutexLocker l(&m_image_lock);
              img0 = lBuffer->GetImage();
              int cv_pixformat = (img0->GetPixelType() == PvPixelYUV422_8)? CV_8UC2: CV_16UC1;

              m_images.enqueue({new Mat(img0->GetHeight(), img0->GetWidth(), cv_pixformat, img0->GetDataPointer()),
                                new Mat(), timestamp, static_cast<int32_t>(minDisparity), pointcloud});
            }

            emit monoReceived();
            break;

          default:
            // Invalid buffer received
            cout << "FMT_ERR(" << consequitive_errors << ") :" << lResult.GetCodeString().GetAscii() << endl;
            consequitive_errors++;
            emit onError(lResult.GetCodeString().GetAscii());
            break;
        }
      } else {
        // Non OK operational result, wait 100ms before retry
        consequitive_errors++;
        emit onError(lOperationResult.GetCodeString().GetAscii());
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
      emit onError(lResult.GetCodeString().GetAscii());
      cout << "BUF_ERR(" << consequitive_errors << ") :" << lResult.GetCodeString().GetAscii() << endl;

      if(lResult.GetCode() == PV_TIMEOUT){
        timeout_count -= 1;
        if (timeout_count == 0){
          timeout_count = MAX_CONS_ERRORS_IN_ACQUISITION;
          emit timeout();
        }
      } else{
          timeout_count = MAX_CONS_ERRORS_IN_ACQUISITION;
      }
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
    m_images.clear();
  }

  // Mark terminated
  emit terminated(consequitive_errors > MAX_CONS_ERRORS_IN_ACQUISITION);
}

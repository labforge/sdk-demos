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

@file util.cc Utility functions implementation for GigE Vision cameras
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#include "gev/util.hpp"
#include "io/util.hpp"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

bool labforge::gev::SetParameter(PvDeviceGEV *aDevice, PvStream *aStream, const char*name, std::variant<int64_t, double, bool, std::string> value) {
  PvGenParameterArray *lDeviceParams = aDevice->GetParameters();
  PvGenParameter *param = nullptr;
  if(lDeviceParams->Get(name) != nullptr) {
    param = lDeviceParams->Get(name);
  }
  if(param == nullptr) {
    lDeviceParams = aDevice->GetCommunicationParameters();
    if(lDeviceParams->Get(name) != nullptr) {
      param = lDeviceParams->Get(name);
    }
  }
  if(param == nullptr) {
    lDeviceParams = aStream->GetParameters();
    if(lDeviceParams->Get(name) != nullptr) {
      param = lDeviceParams->Get(name);
    }
  }
  if(param == nullptr)
    return false;

  PvGenType type;
  param->GetType(type);
  switch(type) {
    case PvGenTypeFloat:
      return dynamic_cast<PvGenFloat*>(param)->SetValue(std::get<double>(value)).IsOK();
    case PvGenTypeInteger:
      return dynamic_cast<PvGenInteger*>(param)->SetValue(std::get<int64_t>(value)).IsOK();
    case PvGenTypeString:
      return dynamic_cast<PvGenString*>(param)->SetValue(std::get<string>(value).c_str()).IsOK();
    case PvGenTypeBoolean:
      return dynamic_cast<PvGenBoolean*>(param)->SetValue(std::get<bool>(value)).IsOK();
    default:
      return false;
  }
}

bool labforge::gev::TweakParameters(PvDeviceGEV *aDevice, PvStream *aStream) {
  // Avoid TOO_MANY_RESENDS
  // see: https://supportcenter.pleora.com/s/article/TOO-MANY-CONSECUTIVE-RESENDS
  // and: https://supportcenter.pleora.com/servlet/fileField?id=0BE34000000CegX
  if(!SetParameter(aDevice, aStream, "ResetOnIdle", 2000)) {
    cout << "NO ResetOnIdle" << endl;
  }
  if(!SetParameter(aDevice, aStream, "ResendDelay", 2000)) {
    cout << "NO ResendDelay" << endl;
  }
  if(!SetParameter(aDevice, aStream, "MaximumResendGroupSize", 60)) {
    cout << "NO MaximumResendGroupSize" << endl;
  }
//  if(!SetParameter(aDevice, aStream, "GevSCPD", 100)) {
//    cout << "NO GevSCPD" << endl;
//  }
  if(!SetParameter(aDevice, aStream, "MaximumPendingResends", 0)) {
    return false;
  }
  if(!SetParameter(aDevice, aStream, "MaximumResendRequestRetryByPacket", 0)) {
    return false;
  }
  // 1.5s for 1 FPS
  if(!SetParameter(aDevice, aStream, "GevMCTT", 1500)) {
    return false;
  }
  // 3s for 1 FPS
  if(!SetParameter(aDevice, aStream, "RequestTimeout", 3000)) {
    return false;
  }
  return true;
}

bool labforge::gev::ConfigureStream(PvDevice *aDevice, PvStream *aStream ) {
  // If this is a GigE Vision device, configure GigE Vision specific streaming parameters
  auto lDeviceGEV = dynamic_cast<PvDeviceGEV *>( aDevice );
  if(!TweakParameters(lDeviceGEV, aStream))
    return false;

  if ( lDeviceGEV != nullptr )
  {
    auto lStreamGEV = dynamic_cast<PvStreamGEV *>( aStream );

    // Negotiate packet size
    PvResult res = lDeviceGEV->NegotiatePacketSize();
    if(!res.IsOK())
      return false;

    // Configure device streaming destination
    res = lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
    return res.IsOK();
  }
  return false;
}

void labforge::gev::CreateStreamBuffers(PvDevice *aDevice, PvStream *aStream, list<PvBuffer *> *aBufferList,
                                        const size_t buffer_count) {
  // Reading payload size from device
  uint32_t lSize = aDevice->GetPayloadSize();

  // Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
  uint32_t lBufferCount = ( aStream->GetQueuedBufferMaximum() < buffer_count ) ?
                          aStream->GetQueuedBufferMaximum() :
                          (uint32_t)buffer_count;

  // Allocate buffers
  for ( uint32_t i = 0; i < lBufferCount; i++ )
  {
    // Create new buffer object
    auto lBuffer = new PvBuffer;

    // Have the new buffer object allocate payload memory
    lBuffer->Alloc( static_cast<uint32_t>( lSize ) );

    // Add to external list - used to eventually release the buffers
    aBufferList->push_back( lBuffer );
  }
}

void labforge::gev::FreeStreamBuffers( list<PvBuffer *> *aBufferList ) {
  // Go through the buffer list
  auto lIt = aBufferList->begin();
  while ( lIt != aBufferList->end() )
  {
    delete *lIt;
    lIt++;
  }

  // Clear the buffer list
  aBufferList->clear();
}

bool labforge::gev::IsEBusLoaded() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  return true; // FIXME: Figure this out in Windows
#else
  return labforge::io::exec("/sbin/lsmod").find("ebUniversalProForEthernet") != string::npos;
#endif
}


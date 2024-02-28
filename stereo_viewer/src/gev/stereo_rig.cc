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

@file stereo_rig.hpp 
@author G. M. Tchamgoue <martin@labforge.ca>
*/

#include "gev/stereo_rig.hpp"
#include <string>
#include <variant>
#include <iostream>

using namespace labforge::gev;
using namespace std;

StereoRig::StereoRig(){
}
StereoRig::StereoRig(PvDevice *lDevice){
  setParameters(lDevice);
}

StereoRig::~StereoRig(){}

static bool getRegister(PvDevice *lDevice, std::string regname, std::variant<int64_t, double> &value){
  bool regOK = false;
  if(lDevice == nullptr) return regOK;

  PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
  PvGenParameter *param = lDeviceParams->Get(regname.c_str());
  if((lDeviceParams == nullptr)||(param == nullptr)) return regOK;

  PvGenType t;
  PvResult res = param->GetType(t);
  if (!res.IsOK()) return regOK;

  if (t == PvGenTypeInteger) { // int
    int64_t ival = 0;
    res = lDeviceParams->GetIntegerValue(param->GetName(), ival);
    regOK = res.IsOK();    
    value = ival;
  } else if (t == PvGenTypeFloat) { // float
    double dval = 0.0;
    res = lDeviceParams->GetFloatValue(param->GetName(), dval);    
    value = dval;   
    regOK = res.IsOK();
  }
  return regOK;
}

void StereoRig::setParameters(PvDevice *lDevice){  
  std::variant<int64_t, double> regvalue;
  const std::string names[] = {"fx", "fy", "cx", "cy", "k1",
                               "k2", "p1", "p2", "k3", "tx",
                               "ty", "tz", "rx", "ry", "rz"};  

  for(std::string name:names){
    for(uint32_t i = 0; i < 2; ++i){ 
      std::string regname = name + std::to_string(i);
      if(getRegister(lDevice, regname, regvalue)){
        std::cout << regname << ": " << std::get<double>(regvalue) << std::endl;
        m_params[regname] = std::get<double>(regvalue);
      }
    }
  }  

  if(getRegister(lDevice, "kWidth", regvalue)){
    std::cout << "kWidth: " << std::get<int64_t>(regvalue) << std::endl;
    m_width = std::get<int64_t>(regvalue);
  }
  if(getRegister(lDevice, "kHeight", regvalue)){
    std::cout << "kHeight: " << std::get<int64_t>(regvalue) << std::endl;
    m_height = std::get<int64_t>(regvalue);
  }

}

bool StereoRig::calibrated(uint32_t width, uint32_t height){
    return ((width == m_width) && (height == m_height) &&
           (m_width > 0) && (m_height > 0));
}

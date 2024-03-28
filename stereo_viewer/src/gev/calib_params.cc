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

@file calib_params.cc
@author G. M. Tchamgoue <martin@labforge.ca>
*/

#include "gev/calib_params.hpp"
#include <string>
#include <variant>
#include <iostream>

using namespace labforge::gev;
using namespace std;

CalibParams::CalibParams(){
}
CalibParams::CalibParams(PvDevice *lDevice){
  setParameters(lDevice);
}

CalibParams::~CalibParams(){}

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

void CalibParams::setParameters(PvDevice *lDevice){
  std::variant<int64_t, double> regvalue;
  const std::string names[] = {"fx", "fy", "cx", "cy", "k1",
                               "k2", "p1", "p2", "k3", "tx",
                               "ty", "tz", "rx", "ry", "rz"};

  uint32_t num_cameras = getRegister(lDevice, "fx1", regvalue)?2:1;

  for(std::string name:names){
    for(uint32_t i = 0; i < num_cameras; ++i){
      std::string regname = name + std::to_string(i);
      if(getRegister(lDevice, regname, regvalue)){
        m_params[regname] = (float)std::get<double>(regvalue);
      } else{
        break;
      }
    }
  }

  if(getRegister(lDevice, "kWidth", regvalue)){
    m_width = (uint32_t)std::get<int64_t>(regvalue);
  }
  if(getRegister(lDevice, "kHeight", regvalue)){
    m_height = (uint32_t)std::get<int64_t>(regvalue);
  }

  if(num_cameras == 2) applyStereoRectify();

}

bool CalibParams::calibrated(uint32_t width, uint32_t height){
  return ((width == m_width) && (height == m_height) &&
          (m_width > 0) && (m_height > 0));
}

void CalibParams::applyStereoRectify(){
  /* K matrices */
  double kmat1[9] = {m_params["fx0"], 0.0, m_params["cx0"], 0.0, m_params["fy0"], m_params["cy0"], 0.0, 0.0, 1.0};
  double kmat2[9] = {m_params["fx1"], 0.0, m_params["cx1"], 0.0, m_params["fy1"], m_params["cy1"], 0.0, 0.0, 1.0};
  cv::Mat K1(3, 3, CV_64F, kmat1);
  cv::Mat K2(3, 3, CV_64F, kmat2);

  /* distortion coeffs */
  double dist1[5] = {m_params["k10"], m_params["k20"], m_params["p10"], m_params["p20"], m_params["k30"]};
  double dist2[5] = {m_params["k11"], m_params["k21"], m_params["p11"], m_params["p21"], m_params["k31"]};
  cv::Mat distCoeffs1(5, 1, CV_64F, dist1);
  cv::Mat distCoeffs2(5, 1, CV_64F, dist2);
  cv::Size imageSize(m_width, m_height);

  /* rotation and translation */
  double rvec[3] = {m_params["rx1"], m_params["ry1"], m_params["rz1"]};
  double tvec[3] = {m_params["tx1"], m_params["ty1"], m_params["tz1"]};
  cv::Mat Rv(3, 1, CV_64F, rvec);
  cv::Mat R(3, 3, CV_64F);
  cv::Mat T(3, 1, CV_64F, tvec);
  cv::Rodrigues(Rv, R);

  cv::stereoRectify(K1, distCoeffs1, K2, distCoeffs2, imageSize,
                    R, T, m_R1, m_R2, m_P1, m_P2, m_Q);
}

void CalibParams::getDepthMatrix(cv::Mat &qmat){
  qmat = m_Q.clone();
}

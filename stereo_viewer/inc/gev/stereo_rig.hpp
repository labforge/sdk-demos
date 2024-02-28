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

#include <cstdint>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <PvDeviceGEV.h>
#include <map>

#ifndef __GEV_STEREO_RIG_HPP__
#define __GEV_STEREO_RIG_HPP__

namespace labforge::gev {

class StereoRig
{    

public:
    StereoRig();
    StereoRig(PvDevice *lDevice);
    ~StereoRig();

    void setParameters(PvDevice *lDevice);
    bool calibrated(uint32_t width, uint32_t height);

private:
    std::map<std::string, float> m_params;
    uint32_t m_width;
    uint32_t m_height; 
};

}

#endif // __GEV_STEREO_RIG_HPP__
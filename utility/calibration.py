"""
******************************************************************************
*  Copyright 2023 Labforge Inc.                                              *
*                                                                            *
* Licensed under the Apache License, Version 2.0 (the "License")            *
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
"""
__author__ = "G. M. Tchamgoue <martin@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

from os import path
import json
import yaml
import numpy as np

def LoadCalibIOParameters(fname='', sensors=0):
    kdata = {}
    
    if not path.isfile(fname) or not fname.lower().endswith(".json"):
        return kdata
    if sensors == 0:
        return kdata
    
    try:
        with open(fname, 'r') as f:
            calib = json.load(f)

        if calib["Calibration"]["cameras"][0]["model"]["polymorphic_name"] != "libCalib::CameraModelOpenCV":
            return kdata
        
        nCameras = len(calib["Calibration"]["cameras"])  
        if nCameras != sensors:
            return kdata

        camera = calib["Calibration"]["cameras"][0]["model"]["ptr_wrapper"]["data"]["CameraModelCRT"]["CameraModelBase"]
        kdata["kWidth"] = camera["imageSize"]["width"]
        kdata["kHeight"] = camera["imageSize"]["height"]  

        for i in range(nCameras):
            id = str(i)
            intrinsics = calib["Calibration"]["cameras"][i]["model"]["ptr_wrapper"]["data"]["parameters"]
            
            kdata["fx" + id] = intrinsics["f"]["val"]
            kdata["fy" + id] = intrinsics["f"]["val"]
            kdata["cx" + id] = intrinsics["cx"]["val"]
            kdata["cy" + id] = intrinsics["cy"]["val"]
            kdata["k1" + id] = intrinsics["k1"]["val"]
            kdata["k2" + id] = intrinsics["k2"]["val"]
            kdata["k3" + id] = intrinsics["k3"]["val"]    
            kdata["p1" + id] = intrinsics["p1"]["val"]
            kdata["p2" + id] = intrinsics["p2"]["val"]
            
            transform = calib["Calibration"]["cameras"][i]["transform"]
            r = transform["rotation"]
            t = transform["translation"]
            kdata["rx" + id] = r["rx"]
            kdata["ry" + id] = r["ry"] 
            kdata["rz" + id] = r["rz"]
            kdata["tx" + id] = t["x"]
            kdata["ty" + id] = t["y"] 
            kdata["tz" + id] = t["z"]   

    except Exception:
        return kdata.clear()

    return kdata

def LoadFlatJSONParameters(fname='', sensors=0):
    kdata = {}
    
    if not path.isfile(fname) or not fname.lower().endswith(".json"):
        return kdata
    if sensors == 0:
        return kdata
    
    try:
        with open(fname, 'r') as f:
            calib = json.load(f)

        if len(calib.keys()) != sensors:
            return kdata
        
        nCameras = 2
        cam_cnt = 0
        kdata["kWidth"] = calib["width"]
        kdata["kHeight"] = calib["height"]  

        for i in range(nCameras):
            id = str(i)
            nodeID = "cam" + id

            if nodeID in calib.keys():
                cam = calib[nodeID]
                
                kdata["fx" + id] = cam["f"] if "f" in cam.keys() else cam["fx"]
                kdata["fy" + id] = cam["f"] if "f" in cam.keys() else cam["fy"]
                kdata["cx" + id] = cam["cx"]
                kdata["cy" + id] = cam["cy"]
                kdata["k1" + id] = cam["k1"]
                kdata["k2" + id] = cam["k2"] if "k2" in cam.keys() else 0.0
                kdata["k3" + id] = cam["k3"] if "k3" in cam.keys() else 0.0    
                kdata["p1" + id] = cam["p1"] if "p1" in cam.keys() else 0.0
                kdata["p2" + id] = cam["p2"] if "p2" in cam.keys() else 0.0
                kdata["rx" + id] = cam["rx"]
                kdata["ry" + id] = cam["ry"] 
                kdata["rz" + id] = cam["rz"]
                kdata["tx" + id] = cam["tx"]
                kdata["ty" + id] = cam["ty"] 
                kdata["tz" + id] = cam["tz"]  
                
                cam_cnt += 1  
        
        if cam_cnt != sensors:
            return kdata.clear()
        

    except Exception:
        return kdata.clear()

    return kdata

def rodrigues(matrix):
    epsilon = 1e-8  # Small value to handle division by zero
    
    # Extract the matrix components
    R = np.array(matrix)
    R_trace = np.trace(R)
    R_diff = R - R.T
    
    # Calculate the angle and axis of rotation
    theta = np.arccos((R_trace - 1) / 2)
    axis = np.array([R_diff[2, 1], R_diff[0, 2], R_diff[1, 0]])
    
    # Normalize the axis
    norm_axis = np.linalg.norm(axis)
    if norm_axis < epsilon:
        # Handle the case when norm_axis is close to zero
        return np.zeros(3)
    
    axis /= norm_axis
    
    # Convert the axis-angle representation to a rotation vector
    rotation_vector = theta * axis
    
    return rotation_vector

def LoadKalibrParameters(fname='', sensors=0):
    kdata = {}
    
    if not path.isfile(fname) or (\
       not fname.lower().endswith(".yaml") and \
       not fname.lower().endswith(".yml")):
        return kdata
    if sensors == 0:
        return kdata
    
    try:
        with open(fname, "r") as f:
            kalibr = yaml.safe_load(f)
        
        nCameras =  len(kalibr.keys())
        if nCameras != sensors:
            return kdata

        for cam in kalibr.keys():
            cam_model = kalibr[cam]["camera_model"]            
            if cam_model != 'pinhole' or cam not in ['cam0', 'cam1']:
                return kdata.clear()
            
            id = cam[-1]
            fu, fv, pu, pv = kalibr[cam]["intrinsics"]
            kdata["fx" + id] = fu
            kdata["fy" + id] = fv
            kdata["cx" + id] = pu
            kdata["cy" + id] = pv

            dist = kalibr[cam]["distortion_coeffs"]
            kdata["k1" + id] = dist[0]
            kdata["k2" + id] = dist[1]           
            kdata["p1" + id] = dist[2]
            kdata["p2" + id] = dist[2] 
            kdata["k3" + id] = 0.0

            if 'T_cn_cnm1' in kalibr[cam].keys():
                T_01 = np.linalg.inv(np.array(kalibr[cam]["T_cn_cnm1"]))
                R = T_01[:3,:3]
                tvec = T_01[:3,3]                
                rvec = rodrigues(R)                
            else:
                rvec = tvec = [0.0, 0.0, 0.0]    
                
            kdata["tx" + id] = tvec[0]
            kdata["ty" + id] = tvec[1]
            kdata["tz" + id] = tvec[2]             
            kdata["rx" + id] = rvec[0]
            kdata["ry" + id] = rvec[1] 
            kdata["rz" + id] = rvec[2]

            width, height = kalibr[cam]["resolution"]
            kdata["kWidth"] = width
            kdata["kHeight"] = height

    except Exception:
        return kdata.clear()
     
    return kdata

def LoadFlatYamlParameters(fname='', sensors=0):
    kdata = {}
    
    if not path.isfile(fname) or (\
       not fname.lower().endswith(".yaml") and \
       not fname.lower().endswith(".yml")):
        return kdata
    if sensors == 0:
        return kdata
    
    try:
        with open(fname, "r") as f:
            calib = yaml.safe_load(f)
        
        nCameras =  len(calib.keys())
        if nCameras != sensors:
            return kdata
        tvec_count = 0
        rvec_count = 0
        for cam in calib.keys():                     
            if cam not in ['cam0', 'cam1']:
                return kdata.clear()
            
            id = cam[-1]            
            kdata["fx" + id] = calib[cam]['fx']
            kdata["fy" + id] = calib[cam]['fy']
            kdata["cx" + id] = calib[cam]['cx']
            kdata["cy" + id] = calib[cam]['cy']
            
            kdata["k1" + id] = calib[cam]['k1']
            kdata["k2" + id] = calib[cam]["k2"] if "k2" in calib[cam].keys() else 0.0
            kdata["k3" + id] = calib[cam]["k3"] if "k3" in calib[cam].keys() else 0.0    
            kdata["p1" + id] = calib[cam]["p1"] if "p1" in calib[cam].keys() else 0.0
            kdata["p2" + id] = calib[cam]["p2"] if "p2" in calib[cam].keys() else 0.0
            
            tvec = rvec = [0.0,0.0,0.0]
            if "tvec" in calib[cam].keys():
                tvec = calib[cam]['tvec']   
                tvec_count += 1 
            if "rvec" in calib[cam].keys():                 
                rvec = calib[cam]['rvec']   
                rvec_count += 1          
            
            kdata["tx" + id] = tvec[0]
            kdata["ty" + id] = tvec[1]
            kdata["tz" + id] = tvec[2]             
            kdata["rx" + id] = rvec[0]
            kdata["ry" + id] = rvec[1] 
            kdata["rz" + id] = rvec[2]
            
            kdata["kWidth"] = calib[cam]['width']
            kdata["kHeight"] = calib[cam]['height']
            
        if tvec_count < (sensors - 1) or rvec_count < (sensors - 1) :
            return kdata.clear()            
    except Exception:
        return kdata.clear()
     
    return kdata

def LoadCalibration(fname='', sensors=0):    
    kdata = LoadCalibIOParameters(fname=fname, sensors=sensors)
    if not bool(kdata):
        kdata = LoadFlatJSONParameters(fname=fname, sensors=sensors)
        if not bool(kdata):
            kdata = LoadKalibrParameters(fname=fname, sensors=sensors)
            if not bool(kdata):
                kdata = LoadFlatYamlParameters(fname=fname, sensors=sensors)
    return kdata 



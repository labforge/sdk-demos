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

def LoadCalibIOParameters(fname=''):
    kdata = dict()
    
    if not path.isfile(fname) or not fname.lower().endswith(".json"):
        return kdata

    try:
        with open(fname, 'r') as f:
            calib = json.load(f)

        if calib["Calibration"]["cameras"][0]["model"]["polymorphic_name"] != "libCalib::CameraModelOpenCV":
            return kdata
        
        nCameras = len(calib["Calibration"]["cameras"])  
        if nCameras < 1:
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

    except Exception as e:
        return kdata.clear()

    return kdata

def LoadFlatJSONParameters(fname=''):
    kdata = dict()
    
    if not path.isfile(fname) or not fname.lower().endswith(".json"):
        return kdata

    try:
        with open(fname, 'r') as f:
            calib = json.load(f)

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
        
        if cam_cnt < 1:
            return kdata.clear()
        

    except Exception as e:
        return kdata.clear()

    return kdata

def rodrigues(r):    
    u, d, v = np.linalg.svd(r)
    r = np.dot(u, v)
    rx = r[2, 1] - r[1, 2]
    ry = r[0, 2] - r[2, 0]
    rz = r[1, 0] - r[0, 1]
    s = np.linalg.norm(np.array([rx, ry, rz])) * np.sqrt(0.25)
    c = np.clip((np.sum(np.diag(r)) - 1) * 0.5, -1, 1)
    theta = np.arccos(c)

    r_out = np.zeros((3, 1))
    if s < 1e-5:
        if c > 0:
            pass #r_out = np.zeros((3, 1))
        else:
            rx, ry, rz = np.clip(np.sqrt((np.diag(r) + 1) * 0.5), 0, np.inf)
            if r[0, 1] < 0:
                ry = -ry
            if r[0, 2] < 0:
                rz = -rz
            if np.abs(rx) < np.abs(ry) and np.abs(rx) < np.abs(rz) and ((r[1, 2] > 0) != (ry * rz > 0)):
                rz = -rz

            r_out = np.array([[rx, ry, rz]]).T
            theta /= np.linalg.norm(r_out)
            r_out *= theta
    return r_out

def LoadKalibrParameters(fname=''):
    kdata = dict()
    import pdb; pdb.set_trace()
    if not path.isfile(fname) or \
       not fname.lower().endswith(".yaml") or \
       not fname.lower().endswith(".yml"):
        return kdata

    try:
        with open(fname, "r") as f:
            kalibr = yaml.safe_load(f)
        
        nCameras =  len(kalibr.keys())
        if nCameras < 1:
            return kdata

        for cam in kalibr.keys():
            cam_model = kalibr[cam]["camera_model"]
            if cam_model != 'pinhole':
                return kdata.clear()
            
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
                kdata["tx" + id] = tvec[0]
                kdata["ty" + id] = tvec[1]
                kdata["tz" + id] = tvec[2] 
                
                rvec = rodrigues(R)
                kdata["rx" + id] = rvec[0]
                kdata["ry" + id] = rvec[1] 
                kdata["rz" + id] = rvec[2]

            width, height = kalibr[cam]["resolution"]
            kdata["kWidth"] = width
            kdata["kHeight"] = height

    except Exception as e:
        return kdata.clear()
     
    return kdata

def LoadCalibration(fname=''):    
    kdata = LoadCalibIOParameters(fname=fname)
    if not bool(kdata):
        kdata = LoadFlatJSONParameters(fname=fname)
        if not bool(kdata):
            kdata = LoadKalibrParameters(fname=fname)

    return kdata 



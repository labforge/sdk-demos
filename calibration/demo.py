# coding: utf-8
"""
******************************************************************************
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
"""
__author__ = "G. M. Tchamgoue <martin@labforge.ca>"
__copyright__ = "Copyright 2024, Labforge Inc."

# reference common utility files
import sys

import argparse
from os import path
import yaml

sys.path.insert(1, '../common')
from connection import init_bottlenose, deinit_bottlenose

import eBUS as eb


def parse_args():
    """
    parses and return the command-line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--mac", default=None, help="MAC address of the Bottlenose camera")
    parser.add_argument("-f", "--kfile", help="path to YAML calibration file")
    return parser.parse_args()


def get_num_cameras(device: eb.PvDeviceGEV):
    """
    Queries the device to determine the number of cameras.
    :param device: The device to query
    :return 1 for Mono and 2 for stereo, 0 for invalid
    """
    reg = device.GetParameters().Get('DeviceModelName')
    res, model = reg.GetValue()
    num_cameras = 0

    if res.IsOK():
        num_cameras = 1 if model[-1].upper() == 'M' else 2

    return num_cameras


def load_calibration(kfile: str, num_cameras: int):
    """
    Load calibration data from the input file
    :param kfile: The input calibration with the Labforge YAML structure.
                  This files has two nodes [cam0, cam1] for stereo and only cam0 for mono.
                  each node has the following structure
                  
                  cam0:
                    fx: 1987.056256568158
                    fy: 1987.056256568158
                            
                    cx: 1093.500298952126
                    cy: 866.9088903543686        
                    k1: -0.14981977634391333
                    k2: -0.08593589340885537    
                                
                    tvec: [0.0, 0.0, 0.0]        
                    rvec: [0.0, 0.0, 0.0]
                    width: 1920
                    height: 1440

    :param num_cameras: The number of calibrated camera nodes to expect from the file.              
    return a dict of all parameters, each key corresponds to a register name on the camera.
    """
    assert path.isfile(kfile), 'Invalid input file'
    assert 0 < num_cameras < 3, 'Invalid number of camera'

    kdata = {}
    try:
        with open(kfile, "r", encoding='UTF-8') as f:
            calib = yaml.safe_load(f)

        if len(calib.keys()) != num_cameras:
            raise Exception('Invalid number of cameras')

        if list(calib.keys()) != ['cam0', 'cam1']:
            raise Exception('Invalid calibration file: Camera node not found')

        if calib['cam0']['width'] != calib['cam1']['width'] or \
           calib['cam0']['height'] != calib['cam1']['height']:
            raise Exception('Mismatching width and height')

        for cam in calib.keys():
            cam_id = cam[-1]
            kdata["fx" + cam_id] = calib[cam]['fx']
            kdata["fy" + cam_id] = calib[cam]['fy']
            kdata["cx" + cam_id] = calib[cam]['cx']
            kdata["cy" + cam_id] = calib[cam]['cy']

            kdata["k1" + cam_id] = calib[cam]['k1']
            kdata["k2" + cam_id] = calib[cam]["k2"] if "k2" in calib[cam].keys() else 0.0
            kdata["k3" + cam_id] = calib[cam]["k3"] if "k3" in calib[cam].keys() else 0.0
            kdata["p1" + cam_id] = calib[cam]["p1"] if "p1" in calib[cam].keys() else 0.0
            kdata["p2" + cam_id] = calib[cam]["p2"] if "p2" in calib[cam].keys() else 0.0

            kdata["tx" + cam_id] = calib[cam]['tvec'][0]
            kdata["ty" + cam_id] = calib[cam]['tvec'][1]
            kdata["tz" + cam_id] = calib[cam]['tvec'][2]
            kdata["rx" + cam_id] = calib[cam]['rvec'][0]
            kdata["ry" + cam_id] = calib[cam]['rvec'][1]
            kdata["rz" + cam_id] = calib[cam]['rvec'][2]

            kdata["kWidth"] = calib[cam]['width']
            kdata["kHeight"] = calib[cam]['height']

    except Exception as e:
        print(e)
        return kdata.clear()

    return kdata


def set_register(device: eb.PvDeviceGEV, regname: str, regvalue: float | int | bool):
    """
    set register on the device
    :param device: The Bottlenose device
    :param regname: the register name to be set.
    :param regvalue: the value to be set into register.
    return True if register properly set, False otherwise
    """

    reg = device.GetParameters().Get(regname)
    if not reg:
        return False
    res, regtype = reg.GetType()
    if not res.IsOK():
        return False
    if regtype == eb.PvGenTypeFloat:
        assert isinstance(regvalue, float) or \
               isinstance(regvalue, int), 'Invalid float register datatype'
        res = reg.SetValue(regvalue)
    elif regtype == eb.PvGenTypeInteger:
        assert isinstance(regvalue, int), 'Invalid integer register datatype'
        res = reg.SetValue(int(regvalue))
    elif regtype == eb.PvGenTypeCommand:
        assert isinstance(regvalue, bool), 'Invalid boolean register'
        if regvalue:
            res = reg.Execute()
    else:
        return False
    return res.IsOK()


def set_calibration_parameters(device: eb.PvDeviceGEV, kparams: dict):
    """
    set the calibration parameters on a Bottlenose device
    :param device: The Bottlenose device
    :param kparams: A dictionary of parameters. Each key is a register name on the device.
    return True if calibration data loaded, False otherwise
    """

    for regname, regvalue in kparams.items():
        if not set_register(device, regname, regvalue):
            return False

    return set_register(device, 'saveCalibrationData', True)


if __name__ == '__main__':
    args = parse_args()

    device, stream, buffers = init_bottlenose(args.mac, True)
    if device is not None:
        num_cameras = get_num_cameras(device)
        print(num_cameras)

        kparams = load_calibration(args.kfile, num_cameras)
        if set_calibration_parameters(device, kparams):
            print('Camera Calibration parameters loaded!')
        else:
            print('Calibration Failed!')

    deinit_bottlenose(device, stream, buffers)

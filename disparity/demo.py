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

import sys
import warnings
import argparse
import eBUS as eb
import cv2
import numpy as np

# reference common utility files
sys.path.insert(1, '../common')
from connection import init_bottlenose, deinit_bottlenose


def parse_args():
    """
    parses and return the command-line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--mac", default=None, help="MAC address of the Bottlenose camera")
    parser.add_argument("-y", "--offsety1", type=int, default=440, choices=range(0, 880),
                        help="Matcher vertical search range.")
    parser.add_argument("-i", "--image", action='store_true',
                        help="Set this to stream the left image with disparity")

    parser.add_argument("-c", "--cost_path", default='Direction4', choices=['Direction4', 'Direction8'],
                        help="disparity cost path")
    parser.add_argument("--crosscheck", action='store_true', help="Set crosscheck value")
    parser.add_argument("--subpixel", default='Direction4', choices=['PolygonLine', 'Parabola'],
                        help="Set sub-pixel value")
    parser.add_argument("--blockwidth", type=int, default=3, choices=range(0, 4), help="block size width")
    parser.add_argument("--blockheight", type=int, default=3, choices=range(0, 3), help="block size height")
    parser.add_argument("--mindisparity", type=int, default=0, choices=range(-127, 127), help="minimum disparity")
    parser.add_argument("--numdisparity", type=int, default=0, choices=range(32, 128), help="number of disparity")
    parser.add_argument("--vsearch", action='store_true', help="Set vertical search")
    parser.add_argument("--threshold", type=int, default=0, choices=range(0, 30), help="set threshold")
    parser.add_argument("--uniqueness", type=int, default=90, choices=range(0, 100), help="set uniqueness ratio")
    parser.add_argument("--penalty1", type=int, default=16, choices=range(0, 127), help="set penalty1")
    parser.add_argument("--penalty2", type=int, default=128, choices=range(0, 461), help="set penalty2")

    return parser.parse_args()


def is_stereo(device: eb.PvDeviceGEV):
    """
    Queries the device to determine if the camera is stereo.
    :param device: The device to query
    :return True if stereo, False otherwise
    """
    reg = device.GetParameters().Get('DeviceModelName')
    res, model = reg.GetValue()
    num_cameras = 0

    if res.IsOK():
        num_cameras = 1 if model[-1].upper() == 'M' else 2

    return num_cameras == 2


def check_stereo(device: eb.PvDeviceGEV, image: bool):
    """
    Checks that the device is a stereo camera and turns on multipart on if needed
    :param device: The device to enable stereo on
    :param image: True to request left image, by turning multipart on
    """
    if not is_stereo(device):
        raise RuntimeError('Sparse3D supported only on Bottlenose stereo.')

    # Get device parameters
    device_params = device.GetParameters()

    # turn multipart on
    multipart = device_params.Get('GevSCCFGMultiPartEnabled')
    multipart.SetValue(image)

    # Turn on rectification
    rectify = device_params.Get("Rectification")
    rectify.SetValue(True)

    # Turn on undistortion
    undistort = device_params.Get("Undistortion")
    undistort.SetValue(True)


def enable_disparity(device: eb.PvDeviceGEV):
    """
    Enable disparity on device
    :param device: The device to enable disparity
    """
    # Get device parameters
    device_params = device.GetParameters()

    # set pixel format
    pixel_format = device_params.Get("PixelFormat")
    pixel_format.SetValue("Coord3D_C16")


def set_y1_offset(device: eb.PvDeviceGEV, value: int):
    """
    Set OffsetY1 register to align the left and right images vertically
    :param device: The device to enable feature points on
    :param value: The Y1 offset parameter
    """

    # Get device parameters
    device_params = device.GetParameters()

    # set register
    y1_offset = device_params.Get("OffsetY1")
    y1_offset.SetValue(value)


def configure_disparity(device: eb.PvDeviceGEV, params=None):
    """
    Configure disparity parameters
    :param device: the device on which to set parameters
    :param params: the set of parameters
    """

    # Get device parameters
    device_params = device.GetParameters()

    # set cost path
    cost_path = device_params.Get("CostPath")
    cost_path.SetValue(params.cost_path)

    # set Crosscheck
    crosscheck = device_params.Get("Crosscheck")
    crosscheck.SetValue(params.crosscheck)

    # set SubPixel
    subpixel = device_params.Get("SubPixel")
    subpixel.SetValue(params.subpixel)

    # set BlockWidth
    block_width = device_params.Get("BlockWidth")
    block_width.SetValue(params.blockwidth)

    # set BlockHeight
    block_height = device_params.Get("BlockHeight")
    block_height.SetValue(params.blockheight)

    # set MinimumDisparity
    min_disparity = device_params.Get("MinimumDisparity")
    min_disparity.SetValue(params.mindisparity)

    # set MinimumDisparity
    num_disparity = device_params.Get("NumberDisparity")
    num_disparity.SetValue(params.numdisparity)

    # set VSearch
    vsearch = device_params.Get("VSearch")
    vsearch.SetValue(params.vsearch)

    # set Threshold
    threshold = device_params.Get("Threshold")
    threshold.SetValue(params.threshold)

    # set Uniqueness
    uniqueness = device_params.Get("Uniqueness")
    uniqueness.SetValue(params.uniqueness)

    # set penalty1
    penalty1 = device_params.Get("Penalty1")
    penalty1.SetValue(params.penalty1)

    # set Penalty2
    penalty2 = device_params.Get("Penalty2")
    penalty2.SetValue(params.penalty2)


def get_invalid_disparity(device: eb.PvDeviceGEV):
    """
    Return the value associated with invalid disparity
    :param device: the device
    :return: the invalid disparity value
    """

    # Get device parameters
    device_params = device.GetParameters()
    inv_register = device_params.Get("InvalidDisparity")
    _, value = inv_register.GetValue()

    return value


def get_disparity_scale_factor(device: eb.PvDeviceGEV):
    """
    Return the value associated with scale factor for subpixel
    :param device: the device
    :return: the scale factor value
    """

    # Get device parameters
    device_params = device.GetParameters()
    scale_register = device_params.Get("PixelScalingFactor")
    _, value = scale_register.GetValue()

    return value


def color_disparity(disp_image, invalid):
    """
    Applies color on disparity data
    :param disp_image: The incoming disparity data
    :param invalid: invalid disparity value
    :return: a colored disparity map
    """
    disp_map = disp_image.copy()
    disp_map[disp_map == invalid] = 0

    disp_map = cv2.normalize(disp_map, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8UC1)
    disp_map = cv2.applyColorMap(disp_map, cv2.COLORMAP_JET)

    return disp_map


def handle_buffer(pvbuffer: eb.PvBuffer, invalid):
    """
    handles incoming buffer with disparity data
    :param pvbuffer: The incoming buffer
    :param invalid: invalid disparity value
    """
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        disparity = pvbuffer.GetImage()
        disparity_data = disparity.GetDataPointer()
        colored_map = color_disparity(disparity_data, invalid)

        cv2.imshow("Disparity", colored_map)
        print(f" dW: {disparity.GetWidth()} dH: {disparity.GetHeight()} ")

    elif payload_type == eb.PvPayloadTypeMultiPart:
        image = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage()
        disparity = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()

        image_data = image.GetDataPointer()
        disparity_data = disparity.GetDataPointer()

        cv_image = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
        colored_map = color_disparity(disparity_data, invalid)
        display_image = np.hstack((cv_image, colored_map))

        cv2.imshow("Disparity", display_image)
        print(f" iW: {image.GetWidth()} iH: {image.GetHeight()} ", end='')
        print(f" dW: {disparity.GetWidth()} dH: {disparity.GetHeight()} ")


def acquire_data(device, stream, invalid):
    """
    Acquire frames from the device and decode sparse chunk data
    :param device: The device to stream from
    :param stream: The stream to use for streaming
    :param invalid: Invalid disparity value
    """

    # Get device parameters need to control streaming
    device_params = device.GetParameters()

    # Map the GenICam AcquisitionStart and AcquisitionStop commands
    start = device_params.Get("AcquisitionStart")
    stop = device_params.Get("AcquisitionStop")

    # Enable streaming and send the AcquisitionStart command
    device.StreamEnable()
    start.Execute()

    while True:
        # Retrieve next pvbuffer
        result, pvbuffer, operational_result = stream.RetrieveBuffer(1000)
        if result.IsOK():
            if operational_result.IsOK():
                # We now have a valid buffer.
                handle_buffer(pvbuffer, invalid)
                if cv2.waitKey(1) & 0xFF != 0xFF:
                    break
            else:
                # Non OK operational result
                warnings.warn(f"Operational result error. {operational_result.GetCodeString()} "
                              f"({operational_result.GetDescription()})",
                              RuntimeWarning)
            # Re-queue the pvbuffer in the stream object
            stream.QueueBuffer(pvbuffer)
        else:
            # Retrieve pvbuffer failure
            warnings.warn(f"Unable to retrieve buffer. {result.GetCodeString()} "
                          f"({result.GetDescription()})", RuntimeWarning)

    # Tell the Bottlenose to stop sending images.
    stop.Execute()


if __name__ == '__main__':
    args = parse_args()

    bn_device, bn_stream, bn_buffers = init_bottlenose(args.mac, True)
    if bn_device is not None:
        check_stereo(device=bn_device, image=args.image)
        set_y1_offset(device=bn_device, value=args.offsety1)
        enable_disparity(device=bn_device)
        configure_disparity(device=bn_device, params=args)

        invalid_disparity = get_invalid_disparity(device=bn_device)
        scaling_factor = get_disparity_scale_factor(device=bn_device)
        print(f'Invalid Disparity: {invalid_disparity} Scaling Factor: {scaling_factor}')

        acquire_data(device=bn_device, stream=bn_stream, invalid=invalid_disparity)

    deinit_bottlenose(bn_device, bn_stream, bn_buffers)

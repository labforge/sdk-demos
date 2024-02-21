# coding: utf-8
"""
******************************************************************************
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
"""
__author__ = ("Thomas Reidemeister <thomas@labforge.ca>"
              "G. M. Tchamgoue <martin@labforge.ca>")
__copyright__ = "Copyright 2023, Labforge Inc."

import sys
import warnings

import argparse
import cv2
import numpy as np
import eBUS as eb


# reference common utility files
sys.path.insert(1, '../common')
from chunk_parser import decode_chunk
from connection import init_bottlenose, deinit_bottlenose

import draw_chunkdata as chk


def parse_args():
    """
    parses and return the command-line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--mac", default=None, help="MAC address of the Bottlenose camera")
    parser.add_argument("--corner_type", default='FAST', choices=['FAST', 'GFTT'],
                        help="type of corner to detect")
    parser.add_argument("--gftt_detector", default='Harris', choices=['Harris', 'MinEigenValue'],
                        help="Set the detector for GFTT")
    parser.add_argument("--max_features", default=1000, type=int, help="maximum number of features to detect")
    parser.add_argument("--quality_level", default=500, type=int, help="quality level for GFTT")
    parser.add_argument("--min_distance", default=15, type=int, help="minimum distance for GFTT")

    parser.add_argument("-k", "--paramk", type=float, default=0, help="free parameter K for Harris corner")
    parser.add_argument("--threshold", type=int, default=100, help="set threshold for FAST")
    parser.add_argument("--nms", action='store_true', help="use nms for FAST")

    return parser.parse_args()


def parse_validate_args():
    """
    Parse and validate the range of arguments
    :return: the parsed argument object
    """
    params = parse_args()

    max_num = 65535 if params.corner_type == 'FAST' else 8192
    assert 0 < params.max_features <= max_num, f'Invalid max_features, values in [1, {max_num}]'

    assert 0 <= params.threshold <= 255, 'Invalid threshold, values in [0, 255]'
    assert 0 < params.quality_level <= 1023, f'Invalid quality_level, values in [1, 1023]'
    assert 0 <= params.min_distance <= 30, 'Invalid min_distance, values in [0, 30]'
    assert 0 <= params.paramk <= 1.0, 'Invalid paramk, values in [0, 1.0]'

    return params


def is_stereo(device: eb.PvDeviceGEV):
    """
    Queries the device to determine if the camera is stereo.
    :param device: The device to query
    :return True if stereo, False otherwise
    """
    reg = device.GetParameters().Get('DeviceModelName')
    res, model = reg.GetValue()

    assert res.IsOK(), 'Error accessing camera'

    num_cameras = 1 if model[-1].upper() == 'M' else 2

    return num_cameras == 2


def set_image_streaming(device: eb.PvDeviceGEV):
    """
    Checks that the device is a stereo camera and turns on multipart on if needed
    :param device: The device
    """

    stereo = is_stereo(device)

    # Get device parameters
    device_params = device.GetParameters()

    # turn multipart on if stereo
    multipart = device_params.Get('GevSCCFGMultiPartEnabled')
    multipart.SetValue(stereo)

    # set image mode
    pixel_format = device_params.Get("PixelFormat")
    pixel_format.SetValue("YUV422_8")

def handle_buffer(pvbuffer, device):
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        image = pvbuffer.GetImage()
        image_data = image.GetDataPointer()

        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')

        # Bottlenose sends as YUV422
        if image.GetPixelType() == eb.PvPixelYUV422_8:
            image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
            if len(keypoints):
                image_data = chk.draw_keypoints(image_data, keypoints[0])
            cv2.imshow("Keypoints", image_data)

    elif payload_type == eb.PvPayloadTypeMultiPart:
        image0 = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage()
        image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()

        image_data0 = image0.GetDataPointer()
        image_data1 = image1.GetDataPointer()
        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')

        cvimage0 = cv2.cvtColor(image_data0, cv2.COLOR_YUV2BGR_YUY2)
        cvimage1 = cv2.cvtColor(image_data1, cv2.COLOR_YUV2BGR_YUY2)

        cvimage0 = chk.draw_keypoints(cvimage0, keypoints[0])
        cvimage1 = chk.draw_keypoints(cvimage1, keypoints[1])

        display_image = np.hstack((cvimage0, cvimage1))

        cv2.imshow("Keypoints", display_image)


def enable_feature_points(device):
    """
    Enable the feature points chunk data
    :param device: The device to enable feature points on
    """
    # Get device parameters
    device_params = device.GetParameters()

    # Enable keypoint detection and streaming
    chunk_mode = device_params.Get("ChunkModeActive")
    chunk_mode.SetValue(True)

    chunk_selector = device_params.Get("ChunkSelector")
    chunk_selector.SetValue("FeaturePoints")

    chunk_enable = device_params.Get("ChunkEnable")
    chunk_enable.SetValue(True)


def configure_fast9(device, max_num: int = 1000, threshold: int = 20, use_nms: bool = True):
    """
    Configure the Fast keypoint detector
    :param device: Device to configure
    :param max_num: Maximum number of features to consider.
    :param threshold: Quality threshold 0...100
    :param use_nms:  Use non-maximum suppression
    """
    # Get device parameters
    device_params = device.GetParameters()
    corner_type = device_params.Get("KPCornerType")
    corner_type.SetValue("Fast9n")

    max_features = device_params.Get("KPMaxNumber")
    max_features.SetValue(max_num)

    fast_threshold = device_params.Get("KPThreshold")
    fast_threshold.SetValue(threshold)

    fast_nms = device_params.Get("KPUseNMS")
    fast_nms.SetValue(use_nms)


def run_demo(device, stream):
    """
    Run the demo
    :param device: The device to stream from
    :param stream: The stream to use for streaming
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
                handle_buffer(pvbuffer, device)
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
            warnings.warn(f"Unable to retrieve buffer. {result.GetCodeString()} ({result.GetDescription()})",
                          RuntimeWarning)

    # Tell the Bottlenose to stop sending images.
    stop.Execute()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    args = parse_validate_args()

    bn_device, bn_stream, bn_buffers = init_bottlenose(args.mac)
    if bn_device is not None:
        # set device into image streaming mode
        set_image_streaming(device=bn_device)

        # Enable keypoint detection and streaming
        enable_feature_points(device=bn_device)
        configure_fast9(device=bn_device, max_num=args.max_features, threshold=args.threshold, use_nms=args.nms)

        run_demo(bn_device, bn_stream)

    deinit_bottlenose(bn_device, bn_stream, bn_buffers)

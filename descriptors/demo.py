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
__author__ = ("G. M. Tchamgoue <martin@labforge.ca>")
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
    parser.add_argument("--max_features", default=1000, type=int,
                        help="maximum number of features to detect")
    parser.add_argument("--threshold", type=int, default=20, help="set threshold for FAST")
    parser.add_argument("--nms", action='store_true', help="use nms for FAST")

    parser.add_argument("--dlen", type=int, default=120, choices=[120, 128, 256, 486], help="AKAZE descriptor length")
    parser.add_argument("--dwin", type=int, default=20, choices=[20, 30, 40, 60, 80], help="Window size for AKAZE computation")

    return parser.parse_args()


def parse_validate_args():
    """
    Parse and validate the range of arguments
    :return: the parsed argument object
    """
    params = parse_args()

    max_num = 65535
    assert 0 < params.max_features <= max_num, f'Invalid max_features, values in [1, {max_num}]'
    assert 0 <= params.threshold <= 255, 'Invalid threshold, values in [0, 255]'

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


def print_descriptor(desc):
    """
    print out basic data from the descriptor.
    It displays the descriptor data of the first keypoint from each list of keypoints
    The list of keypoints and descriptors are index-matched.
    fid is the frame ID the keypoints are from
        0: LEFT_ONLY, for MONO camera or a stereo transmitting only the left image
        1: RIGHT_ONLY, for a stereo camera transmitting only the right image
        2: LEFT_STEREO, for the left image in a stereo transmission
        3: RIGHT_STEREO, for the left image in a stereo transmission
    :param desc: the descriptor data
    :return:
    """

    if len(desc) > 0:
        print('-')
        for i in range(len(desc)):
            print(f'fid: {desc[i].fid} nbits: {desc[i].nbits} '
                  f'nbytes: {desc[i].nbytes} dkp[0]: {desc[i].data[0]}')


def handle_buffer(pvbuffer, device):
    """
    handles incoming buffer with disparity data
    :param pvbuffer: The incoming buffer
    :param device: the device
    """
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        image = pvbuffer.GetImage()
        image_data = image.GetDataPointer()

        # decode keypoints and descriptors chunkdata
        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
        descr = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureDescriptors')

        # Bottlenose sends as YUV422
        image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
        if len(keypoints):
            image_data = chk.draw_keypoints(image_data, keypoints[0])

        # print descriptors
        print_descriptor(descr)

        cv2.imshow("Keypoints", image_data)

    elif payload_type == eb.PvPayloadTypeMultiPart:
        image0 = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage()
        image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()

        image_data0 = image0.GetDataPointer()
        image_data1 = image1.GetDataPointer()

        # decode keypoints and descriptors chunkdata
        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
        descr = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureDescriptors')

        cvimage0 = cv2.cvtColor(image_data0, cv2.COLOR_YUV2BGR_YUY2)
        cvimage1 = cv2.cvtColor(image_data1, cv2.COLOR_YUV2BGR_YUY2)

        if len(keypoints) == 2:
            cvimage0 = chk.draw_keypoints(cvimage0, keypoints[0])
            cvimage1 = chk.draw_keypoints(cvimage1, keypoints[1])

        # print descriptors
        print_descriptor(descr)

        display_image = np.hstack((cvimage0, cvimage1))

        cv2.imshow("Keypoints", display_image)


def enable_chunkdata(device, name):
    """
    Enables chunk data on device
    :param device: The device to enable chunk data on
    :param name: the name of the chunk to enable
    """
    # Get device parameters
    device_params = device.GetParameters()

    # Enable keypoint detection and streaming
    chunk_mode = device_params.Get("ChunkModeActive")
    chunk_mode.SetValue(True)

    chunk_selector = device_params.Get("ChunkSelector")
    chunk_selector.SetValue(name)

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


def configure_descriptor(device, length: int, win: int):
    """
    Sets descriptor parameters on the device
    :param device: The device on which to set parameters
    :param length: The length of the descriptor
    :param win: the size of the window
    :return:
    """
    # Get device parameters
    device_params = device.GetParameters()

    # set descriptor length
    descr_len = device_params.Get("AKAZELength")
    descr_len.SetValue(f"{length}-Bits")

    # set descriptor window
    descr_win = device_params.Get("AKAZEWindow")
    descr_win.SetValue(f"{win}x{win}-Window")


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
            warnings.warn(f"Unable to retrieve buffer. {result.GetCodeString()} "
                          f"({result.GetDescription()})", RuntimeWarning)

    # Tell the Bottlenose to stop sending images.
    stop.Execute()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    args = parse_validate_args()

    bn_device, bn_stream, bn_buffers = init_bottlenose(args.mac)
    if bn_device is not None:
        # set device into image streaming mode
        set_image_streaming(device=bn_device)

        # Enable chunk data
        enable_chunkdata(device=bn_device, name='FeaturePoints')
        enable_chunkdata(device=bn_device, name='FeatureDescriptors')

        # configure fast feature point detector
        configure_fast9(device=bn_device, max_num=args.max_features,
                        threshold=args.threshold, use_nms=args.nms)

        # configure descriptor detection
        configure_descriptor(device=bn_device, length=args.dlen, win=args.dwin)

        # run demo
        run_demo(bn_device, bn_stream)

    deinit_bottlenose(bn_device, bn_stream, bn_buffers)

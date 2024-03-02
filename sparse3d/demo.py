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
import math


# reference common utility files
sys.path.insert(1, '../common')
from chunk_parser import decode_chunk
from connection import init_bottlenose, deinit_bottlenose


def parse_args():
    """
    parses and return the command-line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--mac", default=None, help="MAC address of the Bottlenose camera")
    parser.add_argument("-k", "--max_keypoints", type=int, default=1000, choices=range(1, 65535),
                        help="Maximum number of keypoints to detect")
    parser.add_argument("-t", "--fast_threshold", type=int, default=20, choices=range(0, 255),
                        help="Keypoint threshold for the Fast9 algorithm")
    parser.add_argument("-x", "--match_xoffset", type=int, default=0, choices=range(-4095, 4095),
                        help="Matcher horizontal search range.")
    parser.add_argument("-y", "--match_yoffset", type=int, default=0, choices=range(-4095, 4095),
                        help="Matcher vertical search range.")
    parser.add_argument("-y1", "--offsety1", type=int, default=440, choices=range(0, 880),
                        help="Matcher vertical search range.")
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


def turn_on_stereo(device: eb.PvDeviceGEV):
    """
    Checks that the device is a stereo camera and turns on stereo streaming
    :param device: The device to enable feature points on
    """
    if not is_stereo(device):
        raise RuntimeError('Sparse3D supported only on Bottlenose stereo.')

    # Get device parameters
    device_params = device.GetParameters()

    # set pixel format
    pixel_format = device_params.Get("PixelFormat")
    pixel_format.SetValue("YUV422_8")

    # turn multipart on
    multipart = device_params.Get('GevSCCFGMultiPartEnabled')
    multipart.SetValue(True)


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


def configure_fast9(device: eb.PvDeviceGEV, kp_max: int, threshold: int):
    """
    Configure the Fast9n keypoint detector
    :param device: Device to configure
    :param kp_max: Maximum number of keypoints to consider.
    :param threshold: Quality threshold 0...100
    """
    # Get device parameters
    device_params = device.GetParameters()

    # set FAST9 as keypoint detector
    kp_type = device_params.Get("KPCornerType")
    kp_type.SetValue("Fast9n")

    # set max number of keypoints
    kp_max_num = device_params.Get("KPMaxNumber")
    kp_max_num.SetValue(kp_max)

    # set threshold on detected keypoints
    fast_threshold = device_params.Get("KPThreshold")
    fast_threshold.SetValue(threshold)


def configure_matcher(device: eb.PvDeviceGEV, offsetx: int, offsety: int):
    """
    configure keypoint matcher
    :param device: The device to enable matching on
    :param offsetx: The x offset of the matcher
    :param offsety: The y offset of the matcher
    """
    # Get device parameters
    device_params = device.GetParameters()

    # set x offset parameter
    x_offset = device_params.Get("HAMATXOffset")
    x_offset.SetValue(offsetx)

    # set y offset parameter
    y_offset = device_params.Get("HAMATYOffset")
    y_offset.SetValue(offsety)


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


def enable_sparse_pointcloud(device: eb.PvDeviceGEV):
    """
    Enable sparse point cloud chunk data
    :param device: The device to enable matching on
    """
    # Get device parameters
    device_params = device.GetParameters()

    # Activate chunk streaming
    chunk_mode = device_params.Get("ChunkModeActive")
    chunk_mode.SetValue(True)

    # access chunk registers
    chunk_selector = device_params.Get("ChunkSelector")
    chunk_enable = device_params.Get("ChunkEnable")

    # Enable sparse point cloud
    chunk_selector.SetValue("SparsePointCloud")
    chunk_enable.SetValue(True)


def process_points(keypoints, pc, image, timestamp, min_depth=0.0, max_depth=2.5):
    ply_filename = f'{timestamp}_output_point_cloud.ply'
    image_filename = f'{timestamp}_matched_points.png'
    intermediate_list = []
    valid_points = []
    # Point data
    for i in range(len(keypoints[0]["data"])):
        x = keypoints[0]["data"][i].x
        y = keypoints[0]["data"][i].y
        rgb_value = image[y, x]
        if math.isnan(pc[i].x) or math.isnan(pc[i].y) or math.isnan(pc[i].z):
            continue
        if pc[i].z < min_depth or pc[i].z > max_depth:
            continue
        intermediate_list.append((pc[i].x, pc[i].y, pc[i].z, rgb_value[0], rgb_value[1], rgb_value[2]))
        valid_points.append(cv2.KeyPoint(x=x, y=y, size=15))

    # Draw the keypoints on the image
    if len(valid_points) > 0:
        image = cv2.drawKeypoints(image, valid_points, 0, flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
        cv2.imshow('Matched Points', image)
        if cv2.waitKey(1) & 0xFF != 0xFF:
            return False
        cv2.imwrite(image_filename, image)

    with open(ply_filename, 'w') as ply_file:
        # PLY header
        ply_file.write("ply\n")
        ply_file.write("format ascii 1.0\n")
        ply_file.write(f"element vertex {len(intermediate_list)}\n")
        ply_file.write("property float x\n")
        ply_file.write("property float y\n")
        ply_file.write("property float z\n")
        ply_file.write("property uchar red\n")
        ply_file.write("property uchar green\n")
        ply_file.write("property uchar blue\n")
        ply_file.write("end_header\n")
        for point in intermediate_list:
            ply_file.write(f"{point[0]} {point[1]} {point[2]} {point[3]} {point[4]} {point[5]}\n")
        ply_file.write("\n")

    # Save point cloud to a PLY file
    print(f'Point cloud saved to {ply_filename}')
    return True


def handle_buffer(pvbuffer: eb.PvBuffer, device: eb.PvDeviceGEV):
    """
    handles incoming buffer and decodes the associated sparse point cloud chunk data
    :param pvbuffer: The incoming buffer
    :param device: The device to stream from
    """
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeMultiPart:
        # images associated with the buffer
        image0 = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage() # left image
        image_data = image0.GetDataPointer()
        image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
        #image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()

        # Parses the feature points from the buffer
        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')

        # parses sparse point cloud from the buffer
        # returns a list of Point3D(x,y,z). NaN values are set for unmatched points.
        pc = decode_chunk(device=device, buffer=pvbuffer, chunk='SparsePointCloud')
        timestamp = pvbuffer.GetTimestamp()
        if pc is not None and len(pc) > 0 and keypoints is not None and len(keypoints) > 0:
            return process_points(keypoints, pc, image_data, timestamp)
    return True


def acquire_data(device, stream):
    """
    Acquire frames from the device and decode sparse chunk data
    :param device: The device to stream from
    :param stream: The stream to use for streaming
    """
    cv2.namedWindow('Matched Points', cv2.WINDOW_NORMAL)

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
                if not handle_buffer(pvbuffer, device):
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
        turn_on_stereo(device=bn_device)
        set_y1_offset(device=bn_device, value=args.offsety1)
        configure_fast9(device=bn_device, kp_max=args.max_keypoints, threshold=args.fast_threshold)
        configure_matcher(device=bn_device, offsetx=args.match_xoffset, offsety=args.match_yoffset)
        enable_feature_points(device=bn_device)
        enable_sparse_pointcloud(device=bn_device)
        acquire_data(device=bn_device, stream=bn_stream)

    deinit_bottlenose(bn_device, bn_stream, bn_buffers)

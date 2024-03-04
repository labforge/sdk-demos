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
import numpy as np


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


def enable_feature_points_and_matches(device):
    """
    Enable the feature points chunk data
    :param device: The device to enable feature points on
    """
    # Get device parameters
    device_params = device.GetParameters()

    # Enable keypoint detection and streaming
    chunkMode = device_params.Get("ChunkModeActive")
    chunkMode.SetValue(True)

    match_output_format = device_params.Get("HAMATOutputFormat")

    chunkSelector = device_params.Get("ChunkSelector")
    chunkEnable = device_params.Get("ChunkEnable")

    # Enable Feature Points
    chunkSelector.SetValue("FeaturePoints")
    chunkEnable.SetValue(True)

    # Enable Feature Matches
    chunkSelector.SetValue("FeatureMatches")
    chunkEnable.SetValue(True)
    res = match_output_format.SetValue("Coordinate Only")
    assert res.IsOK()


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


def process_points_and_matches(keypoints, matches, pc, image_left, image_right, timestamp, min_depth=0.0, max_depth=2.5):
    ply_filename = f'{timestamp}_output_point_cloud.ply'
    image_filename_left = f'{timestamp}_matched_left_points.png'
    image_filename_right = f'{timestamp}_matched_right_points.png'
    image_filename_correspondence = f'{timestamp}_correspondence.png'
    intermediate_list = []
    valid_points_left = []
    valid_points_right = []

    # Point data
    for i in range(len(keypoints[0]["data"])):
        x = keypoints[0]["data"][i].x
        y = keypoints[0]["data"][i].y
        x1 = matches.points[i].x
        y1 = matches.points[i].y
        rgb_value = image_left[y, x]
        # Just skip bad 3d matches from HMAT
        if x1 == matches.unmatched or y1 == matches.unmatched:
            continue
        # Not a good test for "valid matches", 3d coordinate can be nan for valid matches
        # if math.isnan(pc[i].x) or math.isnan(pc[i].y) or math.isnan(pc[i].z):
        #     continue
        # if pc[i].z < min_depth or pc[i].z > max_depth:
        #     continue
        intermediate_list.append((pc[i].x, pc[i].y, pc[i].z, rgb_value[0], rgb_value[1], rgb_value[2]))
        valid_points_left.append(cv2.KeyPoint(x=x, y=y, size=15))
        valid_points_right.append(cv2.KeyPoint(x=x1, y=y1, size=15))

    # Do not use "unmatched" !!!
    # Could have multiple matches for a single point
    # for point in matches.points:
    #     if point.x != matches.unmatched and point.y != matches.unmatched:
    #         valid_points_right.append(cv2.KeyPoint(x=point.x, y=point.y, size=15))
    # Sanity (will fail
    # if len(valid_points_left) != len(valid_points_right):
    #    import pdb; pdb.set_trace()

    # Draw the keypoints on the image
    if len(valid_points_left) > 0:
        #image = cv2.drawKeypoints(image, valid_points_left, 0, flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
        # Detailled drawing with enumeration
        for image, points in [(image_left, valid_points_left), (image_right, valid_points_right)]:
            for i, kp in enumerate(points):
                x, y = int(kp.pt[0]), int(kp.pt[1])  # Get the coordinates of the keypoint

                # Draw a small circle at the keypoint location
                cv2.circle(image, (x, y), 3, (255, 0, 0), -1)  # Change color and thickness as needed

                # Put a text label (index) next to the circle
                cv2.putText(image, f"{i:02d}", (x + 10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 0, 0), 1)

        cv2.imshow('Matched Points', image_left)
        if cv2.waitKey(1) & 0xFF != 0xFF:
            return False
        cv2.imwrite(image_filename_left, image_left)
        cv2.imwrite(image_filename_right, image_right)

        # Correspondence plot
        good = [cv2.DMatch(_imgIdx=0, _queryIdx=i, _trainIdx=i, _distance=0)
                for i in range(len(valid_points_left))]
        image_correspondence = np.array([])
        image_correspondence = cv2.drawMatches(image_left,
                                  valid_points_left,
                                  image_right,
                                  valid_points_right,
                                  matches1to2=good,
                                  outImg=image_correspondence,
                                  flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
        cv2.imwrite(image_filename_correspondence, image_correspondence)

    # with open(ply_filename, 'w') as ply_file:
    #     # PLY header
    #     ply_file.write("ply\n")
    #     ply_file.write("format ascii 1.0\n")
    #     ply_file.write(f"element vertex {len(intermediate_list)}\n")
    #     ply_file.write("property float x\n")
    #     ply_file.write("property float y\n")
    #     ply_file.write("property float z\n")
    #     ply_file.write("property uchar red\n")
    #     ply_file.write("property uchar green\n")
    #     ply_file.write("property uchar blue\n")
    #     ply_file.write("end_header\n")
    print("===== RESULTS ======")
    for point in intermediate_list:
        print(f"{point[0]} {point[1]} {point[2]} {point[3]} {point[4]} {point[5]}\n")
        # ply_file.write(f"{point[0]} {point[1]} {point[2]} {point[3]} {point[4]} {point[5]}\n")
        # ply_file.write("\n")

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
        image_data_left = image0.GetDataPointer()
        image_data_left = cv2.cvtColor(image_data_left, cv2.COLOR_YUV2BGR_YUY2)
        image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()
        image_data_right = image1.GetDataPointer()
        image_data_right = cv2.cvtColor(image_data_right, cv2.COLOR_YUV2BGR_YUY2)
        print('.')

        # Parses the feature points from the buffer
        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
        if keypoints is not None:
            print('k')

        # Use matches with coordinates instead
        matches = decode_chunk(device=device, buffer=pvbuffer, chunk="FeatureMatches")
        if matches is not None:
            print('m')

        # parses sparse point cloud from the buffer
        # returns a list of Point3D(x,y,z). NaN values are set for unmatched points.
        pc = decode_chunk(device=device, buffer=pvbuffer, chunk='SparsePointCloud')
        if pc is not None:
            print('p')

        timestamp = pvbuffer.GetTimestamp()

        if pc is not None and len(pc) > 0 and keypoints is not None and len(keypoints) > 0 and len(matches) > 0:
            return process_points_and_matches(keypoints,
                                              matches,
                                              pc,
                                              image_data_left,
                                              image_data_right,
                                              timestamp)
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
        enable_feature_points_and_matches(device=bn_device)
        enable_sparse_pointcloud(device=bn_device)
        acquire_data(device=bn_device, stream=bn_stream)

    deinit_bottlenose(bn_device, bn_stream, bn_buffers)

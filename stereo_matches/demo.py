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
__author__ = ("Thomas Reidemeister <thomas@labforge.ca>"
              "G. M. Tchamgoue <martin@labforge.ca>")
__copyright__ = "Copyright 2024, Labforge Inc."

import sys
import warnings

import eBUS as eb
import cv2
import draw_chunkdata as chk

# reference common utility files
sys.path.insert(1, '../common')
from chunk_parser import decode_chunk
from connection import init_bottlenose, deinit_bottlenose


def handle_buffer(pvbuffer, device):
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        raise RuntimeError("Unexpected image payload type, make sure you are running this demo with Bottlenose Stereo")

    elif payload_type == eb.PvPayloadTypeMultiPart:
        image0 = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage()
        image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()
        idata = [image0.GetDataPointer(), image1.GetDataPointer()]
        cvimage = []

        if image0.GetPixelType() == eb.PvPixelYUV422_8 and image1.GetPixelType() == eb.PvPixelYUV422_8:
            for i in range(len(idata)):
                image_data = cv2.cvtColor(idata[i], cv2.COLOR_YUV2BGR_YUY2)
                cvimage.append(image_data)

        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
        if len(keypoints) > 0:
            for i in range(len(idata)):
                cvimage[i] = chk.draw_keypoints(cvimage[i], keypoints[i])
        else:
            print("No keypoints found")

        for i in range(len(idata)):
            cv2.imshow(f"Camera {i}", cvimage[i])

        matches = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureMatches')
        found = False
        if matches is not None:
            print(f"Matches: {len(matches.points)} data: ({matches.points[0].x}, {matches.points[0].y})")
            for i, pt in enumerate(matches.points):
                if (pt.x != matches.unmatched) and (pt.y != matches.unmatched):
                    print(f"Match {i}: ({pt.x}, {pt.y})")
                    found = True
        if not found:
            print("No matches found")
        else:
            import pdb; pdb.set_trace()


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

    chunkSelector = device_params.Get("ChunkSelector")
    chunkEnable = device_params.Get("ChunkEnable")

    # Enable Feature Points
    chunkSelector.SetValue("FeaturePoints")
    chunkEnable.SetValue(True)

    # Enable Feature Matches
    chunkSelector.SetValue("FeatureMatches")
    chunkEnable.SetValue(True)


def configure_fast9(device, max_num=1000, threshold=20, useNonMaxSuppression=True):
    """
    Configure the Fast9n keypoint detector
    :param device: Device to configure
    :param max_num: Maximum number of features to consider.
    :param threshold: Quality threshold 0...100
    :param useNonMaxSuppression:  Use non-maximum suppression
    :return: None
    """
    # Get device parameters
    device_params = device.GetParameters()
    KPCornerType = device_params.Get("KPCornerType")
    KPCornerType.SetValue("Fast9n")

    KPFast9nMaxNum = device_params.Get("KPMaxNumber")
    KPFast9nMaxNum.SetValue(int(max_num))

    KPFast9nThreshold = device_params.Get("KPThreshold")
    KPFast9nThreshold.SetValue(threshold)

    KPFast9nUseNonMaxSuppression = device_params.Get("KPUseNMS")
    KPFast9nUseNonMaxSuppression.SetValue(useNonMaxSuppression)


def run_demo(device, stream, max_fast_features=1000, fast_threshold=10, use_non_max_suppression=True):
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

    # Enable keypoint detection and streaming
    enable_feature_points_and_matches(device)
    configure_fast9(device, max_fast_features, fast_threshold, use_non_max_suppression)

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
    mac_address = None
    if len(sys.argv) >= 2:
        mac_address = sys.argv[1]

    device, stream, buffers = init_bottlenose(mac_address, True)
    if device is not None:
        run_demo(device, stream)

    deinit_bottlenose(device, stream, buffers)

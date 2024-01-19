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
        image = pvbuffer.GetImage()
        image_data = image.GetDataPointer()

        keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')

        # Bottlenose sends as YUV422
        if image.GetPixelType() == eb.PvPixelYUV422_8:
            image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
            if len(keypoints):
                image_data = chk.draw_keypoints(image_data, keypoints[0])
            cv2.imshow("Keypoints", image_data)


def enable_feature_points(device):
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
    chunkSelector.SetValue("FeaturePoints")
    chunkEnable = device_params.Get("ChunkEnable")
    chunkEnable.SetValue(True)


def configure_fast9(device, max_num=1000, threshold=20, useNonMaxSuppression=True):
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
    enable_feature_points(device)
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

    device, stream, buffers = init_bottlenose(mac_address)
    if device is not None:
        run_demo(device, stream)

    deinit_bottlenose(device, stream, buffers)

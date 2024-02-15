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
__author__ = "Thomas Reidemeister <thomas@labforge.ca>"
__copyright__ = "Copyright 2024, Labforge Inc."

import sys
import warnings

import eBUS as eb
import cv2
sys.path.insert(1, '../common')
from connection import init_bottlenose, deinit_bottlenose


def handle_buffer(pvbuffer):
    """
    Handle the buffer and draw the Laplacian Variance on the image.
    :param pvbuffer: Buffer read from the stream
    """
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        image = pvbuffer.GetImage()
        image_data = image.GetDataPointer()

        # Bottlenose sends as YUV422
        if image.GetPixelType() == eb.PvPixelYUV422_8:
            image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
            # compute laplacian
            # Convert image to grayscale
            image_gray = cv2.cvtColor(image_data, cv2.COLOR_BGR2GRAY)
            laplacian = cv2.Laplacian(image_gray, cv2.CV_64F)
            laplacian_variance = laplacian.var()
            cv2.putText(image_data, f"Laplacian variance: {laplacian_variance:5.3f}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2, cv2.LINE_AA)
            cv2.imshow("Source Image", image_data)

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
                handle_buffer(pvbuffer)
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

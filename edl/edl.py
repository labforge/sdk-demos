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
__author__ = "Thomas Reidemeister <thomas@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

import sys
import eBUS as eb
import cv2

# reference common utility files
sys.path.insert(1, '../common')
from chunk_parser import decode_chunk
from connection import init_bottlenose, deinit_bottlenose
import numpy as np


def acquire_images(device, stream, nframes=None):
    # Get device parameters need to control streaming
    device_params = device.GetParameters()

    # Map the GenICam AcquisitionStart and AcquisitionStop commands
    start = device_params.Get("AcquisitionStart")
    stop = device_params.Get("AcquisitionStop")

    pixel_format = device_params.Get("PixelFormat")
    print(pixel_format)
    res = pixel_format.SetValue(eb.PvPixelBayerRG10)
    if not res.IsOK():
        print("Failed to set pixel format to BayerRG10")
        return

    # Get stream parameters
    stream_params = stream.GetParameters()

    # Map a few GenICam stream stats counters
    frame_rate = stream_params.Get("AcquisitionRate")
    bandwidth = stream_params["Bandwidth"]

    # Enable streaming and send the AcquisitionStart command
    print("Enabling streaming and sending AcquisitionStart command.")
    device.StreamEnable()
    start.Execute()

    doodle = "|\\-|-/"
    doodle_index = 0
    display_image = False

    # Acquire images until the user instructs us to stop.
    if nframes is None:
        print("\n<press a key to stop streaming>")

    while (nframes is None) or (nframes > 0):
        # Retrieve next pvbuffer
        result, pvbuffer, operational_result = stream.RetrieveBuffer(1000)
        if result.IsOK():
            if operational_result.IsOK():
                if nframes is not None:
                    nframes = nframes - 1

                result, frame_rate_val = frame_rate.GetValue()
                result, bandwidth_val = bandwidth.GetValue()

                print(f"{doodle[doodle_index]} BlockID: {pvbuffer.GetBlockID()}", end='')

                payload_type = pvbuffer.GetPayloadType()
                if payload_type == eb.PvPayloadTypeImage:
                    image = pvbuffer.GetImage()
                    image_data = image.GetDataPointer()
                    import ipdb; ipdb.set_trace()
                    print(f" W: {image.GetWidth()} H: {image.GetHeight()}", end='')

                    if (image.GetPixelType() == eb.PvPixelBayerRG10) or (image.GetPixelType() == eb.PvPixelBayerBG10):
                        display_image = True
                    if nframes is None:
                        if display_image:
                            cv2.imshow("stream", image_data)
                    if nframes is None:
                        if cv2.waitKey(1) & 0xFF != 0xFF:
                            break
                print(f" {frame_rate_val:.1f} FPS  {bandwidth_val / 1000000.0:.1f} Mb/s", end='\r')
            else:
                # Non OK operational result
                print(f"{doodle[doodle_index]} {operational_result.GetCodeString()}", end='\r')
            # Re-queue the pvbuffer in the stream object
            stream.QueueBuffer(pvbuffer)
        else:
            # Retrieve pvbuffer failure
            print(f"{doodle[doodle_index]} {result.GetCodeString()}      ", end='\r')
        doodle_index = (doodle_index + 1) % 6

    if nframes is None:
        cv2.destroyAllWindows()

    # Tell the Bottlenose to stop sending images.
    print("\nSending AcquisitionStop command to the Bottlenose")
    stop.Execute()


if __name__ == '__main__':
    mac_address = None
    nframes = None
    if len(sys.argv) >= 2:
        mac_address = sys.argv[1]
    if len(sys.argv) >= 3:
        nframes = int(sys.argv[2])

    print("Embedded Data Lines (EDL) and RAW10 example")
    device, stream, buffers = init_bottlenose(mac_address, stereo=False)
    acquire_images(device, stream, nframes)
    deinit_bottlenose(device, stream, buffers)

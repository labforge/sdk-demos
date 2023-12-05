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
import eBUS as eb
import cv2
import numpy as np
from collections import namedtuple
from chunk_parser import decode_chunk

import draw_chunkdata as chk

BUFFER_COUNT = 16
LABFORGE_MAC_RANGE = '8c:1f:64:d0:e'


def find_bottlenose(mac=None):
    """
    Finds a running Bottlenose sensor on the network.
    :param mac: Optional, provide the mac address of the Bottlenose to connect to
    :return: The connection ID of the bottlenose camera, if multiple cameras are found, the first one is returned
    """
    system = eb.PvSystem()
    system.Find()

    # Detect, select Bottlenose.
    device_vector = []
    for i in range(system.GetInterfaceCount()):
        interface = system.GetInterface(i)
        print(f"   {interface.GetDisplayID()}")
        for j in range(interface.GetDeviceCount()):
            device_info = interface.GetDeviceInfo(j)
            if device_info.GetMACAddress().GetUnicode().find(LABFORGE_MAC_RANGE) == 0:
                device_vector.append(device_info)
                print(f"[{len(device_vector) - 1}]\t{device_info.GetDisplayID()}")
                if mac is not None and mac == device_info.GetMACAddress().GetUnicode():
                    return device_info.GetConnectionID()

    if len(device_vector) == 0:
        print(f"No Bottlenose camera found!")
        return None

    # Return first Bottlenose found
    return device_vector[0].GetConnectionID()


def connect_to_device(connection_id):
    # Connect to the GigE Vision or USB3 Vision device
    print("Connecting to device.")
    result, device = eb.PvDevice.CreateAndConnect(connection_id)
    if device is None:
        print(f"Unable to connect to device: {result.GetCodeString()} ({result.GetDescription()})")
    return device


def open_stream(connection_ID):
    # Open stream to the GigE Vision or USB3 Vision device
    print("Opening stream from device.")
    result, stream = eb.PvStream.CreateAndOpen(connection_ID)
    if stream is None:
        print(f"Unable to stream from device. {result.GetCodeString()} ({result.GetDescription()})")
    return stream


def configure_stream(device, stream):
    # If this is a GigE Vision device, configure GigE Vision specific streaming parameters
    if isinstance(device, eb.PvDeviceGEV):
        # Negotiate packet size
        device.NegotiatePacketSize()
        # Configure device streaming destination
        device.SetStreamDestination(stream.GetLocalIPAddress(), stream.GetLocalPort())


def configure_stream_buffers(device, stream):
    buffer_list = []
    # Reading payload size from device
    size = device.GetPayloadSize()

    # Use BUFFER_COUNT or the maximum number of buffers, whichever is smaller
    buffer_count = stream.GetQueuedBufferMaximum()
    if buffer_count > BUFFER_COUNT:
        buffer_count = BUFFER_COUNT

    # Allocate buffers
    for i in range(buffer_count):
        # Create new pvbuffer object
        pvbuffer = eb.PvBuffer()
        # Have the new pvbuffer object allocate payload memory
        pvbuffer.Alloc(size)
        # Add to external list - used to eventually release the buffers
        buffer_list.append(pvbuffer)

    # Queue all buffers in the stream
    for pvbuffer in buffer_list:
        stream.QueueBuffer(pvbuffer)
    print(f"Created {buffer_count} buffers with payload {size}")
    return buffer_list


def activate_stereo(device: eb.PvDeviceGEV, value: bool = True):
    """
    Turns on/off stereo transmission
    """

    is_stereo = False
    model = device.GetParameters().Get("DeviceModelName")

    if model:
        res, name = model.GetValue()
        is_stereo = (res.IsOK() and name.endswith('_ST'))

    value &= is_stereo
    multipart = device.GetParameters().Get("GevSCCFGMultiPartEnabled")
    if multipart:
        multipart.SetValue(value)

def acquire_images(device, stream, nframes=None):
    # Get device parameters need to control streaming
    device_params = device.GetParameters()

    # Map the GenICam AcquisitionStart and AcquisitionStop commands
    start = device_params.Get("AcquisitionStart")
    stop = device_params.Get("AcquisitionStop")

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
    warning_issued = False

    # Acquire images until the user instructs us to stop.
    if nframes is None:
        print("\n<press a key to stop streaming>")

    while (nframes is None) or (nframes > 0):
        # Retrieve next pvbuffer
        result, pvbuffer, operational_result = stream.RetrieveBuffer(1000)
        if result.IsOK():
            if operational_result.IsOK():
                #
                # We now have a valid pvbuffer. This is where you would typically process the pvbuffer.
                # -----------------------------------------------------------------------------------------
                # ...
                if nframes is not None:
                    nframes = nframes - 1

                result, frame_rate_val = frame_rate.GetValue()
                result, bandwidth_val = bandwidth.GetValue()

                print(f"{doodle[doodle_index]} BlockID: {pvbuffer.GetBlockID()}", end='')

                payload_type = pvbuffer.GetPayloadType()
                if payload_type == eb.PvPayloadTypeImage:
                    image = pvbuffer.GetImage()
                    image_data = image.GetDataPointer()
                    print(f" W: {image.GetWidth()} H: {image.GetHeight()} ")

                    keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
                    for kp in keypoints:
                        print(f"Keypoints: fid: {kp['fid']} len: {len(kp['data'])} data: {kp['data'][0]}")

                    descriptors = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureDescriptors')
                    for descr in descriptors:
                        print(f"Descriptors: fid: {descr.fid} num: {descr.num} len: {descr.nbits} data: {descr.data[10]}")

                    bboxes = decode_chunk(device=device, buffer=pvbuffer, chunk='BoundingBoxes')
                    if bboxes is not None:
                        print(f"BBoxes: {len(bboxes)} data: {bboxes[0]}")

                    if image.GetPixelType() == eb.PvPixelMono8:
                        display_image = True

                    # Bottlenose sends as YUV422
                    if image.GetPixelType() == eb.PvPixelYUV422_8:
                        image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
                        display_image = True

                    if nframes is None:
                        if display_image:
                            if len(keypoints):
                                image_data = chk.draw_keypoints(image_data, keypoints[0])
                            cv2.imshow("stream", image_data)

                    else:
                        if not warning_issued:
                            # display a message that video only display for Mono8 / RGB8 images
                            print(f" ")
                            print(f" Currently only Mono8 / RGB8 images are displayed", end='\r')
                            print(f"")
                            warning_issued = True

                    if nframes is None:
                        if cv2.waitKey(1) & 0xFF != 0xFF:
                            break

                elif payload_type == eb.PvPayloadTypeChunkData:
                    print(f" Chunk Data payload type with {pvbuffer.GetChunkCount()} chunks", end='')

                elif payload_type == eb.PvPayloadTypeRawData:
                    print(f" Raw Data with {pvbuffer.GetRawData().GetPayloadLength()} bytes", end='')

                elif payload_type == eb.PvPayloadTypeMultiPart:
                    image0 = pvbuffer.GetMultiPartContainer().GetPart(0).GetImage()
                    image1 = pvbuffer.GetMultiPartContainer().GetPart(1).GetImage()
                    image_data0 = image0.GetDataPointer()
                    image_data1 = image1.GetDataPointer()
                    print(f" W: {image0.GetWidth()} H: {image1.GetHeight()} ", end='')

                    keypoints = decode_chunk(device=device, buffer=pvbuffer, chunk='FeaturePoints')
                    for kp in keypoints:
                        print(f"Keypoints: fid: {kp['fid']} len: {len(kp['data'])} data: {kp['data'][0]}")

                    descriptors = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureDescriptors')
                    for descr in descriptors:
                        print(f"Descriptors: fid: {descr.fid} num: {descr.num} len: {descr.nbits} data: {descr.data[0]}")

                    bboxes = decode_chunk(device=device, buffer=pvbuffer, chunk='BoundingBoxes')
                    if bboxes is not None:
                        print(f"BBoxes: {len(bboxes)} data: {bboxes[0]}")

                    matches = decode_chunk(device=device, buffer=pvbuffer, chunk='FeatureMatches')
                    if matches is not None:
                        print(f"Matches: {len(matches.points)} data: ({matches.points[0].x}, {matches.points[0].y})")

                    pointcloud = decode_chunk(device=device, buffer=pvbuffer, chunk='SparsePointCloud')
                    if pointcloud is not None:
                        print(f"PointCloud: {len(pointcloud)} data: ({pointcloud[0].x}, {pointcloud[0].y}, {pointcloud[0].z})")

                    meta = decode_chunk(device=device, buffer=pvbuffer, chunk='FrameInformation')
                    if meta is not None:
                        print(f"FrameInformation: {meta.real_time}, exposure = {meta.exposure:.2f} ms, gain = {meta.gain:.1f}")

                    # Bottlenose sends as YUV422
                    if image0.GetPixelType() == eb.PvPixelYUV422_8:
                        image_data0 = cv2.cvtColor(image_data0, cv2.COLOR_YUV2BGR_YUY2)
                        image_data1 = cv2.cvtColor(image_data1, cv2.COLOR_YUV2BGR_YUY2)
                        display_image = True

                    if nframes is None:
                        if display_image:
                            if len(keypoints):
                                image_data0 = chk.draw_keypoints(image_data0, keypoints[0])
                                image_data1 = chk.draw_keypoints(image_data1, keypoints[1])

                            cv2.imshow("stream0", image_data0)
                            cv2.imshow("stream1", image_data1)

                    else:
                        if not warning_issued:
                            # display a message that video only display for Mono8 / RGB8 images
                            print(f" ")
                            print(f" Currently only Mono8 / RGB8 images are displayed", end='\r')
                            print(f"")
                            warning_issued = True

                    if nframes is None:
                        if cv2.waitKey(1) & 0xFF != 0xFF:
                            break

                    print(f" Multi Part with {pvbuffer.GetMultiPartContainer().GetPartCount()} parts", end='')

                else:
                    print(" Payload type not supported by this sample", end='')

                print(f" {frame_rate_val:.1f} FPS  {bandwidth_val / 1000000.0:.1f} Mb/s     ", end='\r')
            else:
                # Non OK operational result
                print(f"{doodle[doodle_index]} {operational_result.GetCodeString()}       ", end='\r')
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

    # Disable streaming on the Bottlenose
    print("Disable streaming on the controller.")
    device.StreamDisable()

    # Abort all buffers from the stream and dequeue
    print("Aborting buffers still in stream")
    stream.AbortQueuedBuffers()
    while stream.GetQueuedBufferCount() > 0:
        result, pvbuffer, lOperationalResult = stream.RetrieveBuffer()


if __name__ == '__main__':
    mac_address = None
    nframes = None
    if len(sys.argv) >= 2:
        mac_address = sys.argv[1]
    if len(sys.argv) >= 3:
        nframes = int(sys.argv[2])

    print("Simple Streaming Sample")
    connection_ID = find_bottlenose(mac_address)
    if connection_ID:
        device = connect_to_device(connection_ID)
        if device:
            activate_stereo(device=device, value=True)
            stream = open_stream(connection_ID)
            if stream:
                configure_stream(device, stream)
                buffer_list = configure_stream_buffers(device, stream)
                acquire_images(device, stream, nframes)
                buffer_list.clear()

                # Close the stream
                print("Closing stream")
                stream.Close()
                eb.PvStream.Free(stream)

            # Disconnect the device
            print("Disconnecting Bottlenose")
            device.Disconnect()
            eb.PvDevice.Free(device)

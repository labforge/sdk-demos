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
    multipart = device.GetParameters().Get("GevSCCFGMultiPartEnabled")
    if multipart:
        multipart.SetValue(value)


def read_chunk_id(device: eb.PvDeviceGEV, chunk_name: str):
    """
    Given a chunk_name, returns the associated chunkID if found
    return -1 otherwise
    """
    chunk_selector = device.GetParameters().Get("ChunkSelector")

    chunk_id = -1
    res, chunk_reg = chunk_selector.GetEntryByName(chunk_name)

    if res.IsOK():
        res, reg_id = chunk_reg.GetValue()
        if res.IsOK():
            chunk_id = reg_id

    return chunk_id


def has_chunk_data(buffer: eb.PvBuffer, chunk_id: int):
    """
    Returns true if the input buffer has a chunk that matches the ID
    """
    if not buffer:
        return False

    if not buffer.HasChunks() or chunk_id < 0:
        return False

    for i in range(buffer.GetChunkCount()):
        rs, cid = buffer.GetChunkIDByIndex(i)
        if rs.IsOK() and cid == chunk_id:
            return True

    return False


def decode_chunk_keypoint(data):
    """
    decode the input buffer as keypoints.
    each keypoint (x:uint16, y:uint16)
    each set of keypoints comes from a designated frame
    fid 0: LEFT_ONLY, 1: RIGHT_ONLY, 2: LEFT_STEREO, 3: RIGHT_STEREO
    """
    if len(data) == 0:
        return None, 0

    fields = ['x', 'y']
    Keypoint = namedtuple('Keypoint', fields)

    num_keypoints = int.from_bytes(data[0:2], 'little')
    frame_id = int.from_bytes(data[2:4], 'little')
    if num_keypoints <= 0 or num_keypoints > 0xFFFF:
        return None, 0
    if frame_id not in [0, 1, 2, 3]:
        return None, 0

    chunkdata = [Keypoint(int.from_bytes(data[i:(i + 2)], 'little'),
                          int.from_bytes(data[(i + 2):(i + 4)], 'little'))
                 for i in range(4, (num_keypoints + 1) * 4, 4)]

    offset = 0
    if frame_id in [2, 3]:
        offset = (num_keypoints + 1) * 4

    return {'fid': frame_id, 'data': chunkdata}, offset


def decode_chunk_descriptor(data):
    """
    decode the input buffer as descriptor.
    each descriptor can be up to 64 bytes long
    each set of descriptor corresponds to a set of keypoints and comes from a designated frame
    fid 0: LEFT_ONLY, 1: RIGHT_ONLY, 2: LEFT_STEREO, 3: RIGHT_STEREO
    """
    if len(data) == 0:
        return None, 0

    fields = ['fid', 'nbits', 'nbytes', 'size', 'data']
    Descriptor = namedtuple('Descriptors', fields)

    num_descr = int.from_bytes(data[0:2], 'little')
    frame_id = int.from_bytes(data[2:4], 'little')
    if num_descr <= 0 or num_descr > 0xFFFF:
        return None, 0
    if frame_id not in [0, 1, 2, 3]:
        return None, 0

    len_descr = int.from_bytes(data[4:8], 'little')

    nbytes = 1
    while nbytes < len_descr:
        nbytes <<= 1
    nbytes //= 8

    descr_data = [data[i:(i + nbytes)] for i in range(8, num_descr * 64, 64)]
    chunkdata = Descriptor(frame_id, len_descr, nbytes, num_descr, descr_data)

    offset = 0
    if frame_id in [2, 3]:
        offset = (num_descr * 64) + 8

    return chunkdata, offset


def decode_chunk_bbox(data):
    """
    decode the input buffer as bounding boxes.
    each set of boxes comes from a designated frame
    fid 0: LEFT_ONLY, 1: RIGHT_ONLY, 2: LEFT_STEREO, 3: RIGHT_STEREO
    """
    if len(data) == 0:
        return None, 0

    frame_id = int.from_bytes(data[0:4], 'little')
    num_boxes = int.from_bytes(data[4:8], 'little')
    if num_boxes <= 0 or (num_boxes * 48 + 8) > len(data):
        return None, 0
    if frame_id not in [0, 1, 2, 3]:
        return None, 0

    fields = ['cid', 'score', 'left', 'top', 'right', 'bottom', 'label']
    BBox = namedtuple('BBox', fields)

    chunkdata = []
    for i in range(8, num_boxes * 48, 48):
        cid = int.from_bytes(data[i:i + 4], 'little')
        score = np.frombuffer(data[i + 4:i + 8], dtype=np.float32)[0]
        left = int.from_bytes(data[i + 8:i + 12], 'little')
        top = int.from_bytes(data[i + 12:i + 16], 'little')
        right = int.from_bytes(data[i + 16:i + 20], 'little')
        bottom = int.from_bytes(data[i + 20:i + 24], 'little')
        label = bytearray(data[i + 24:i + 48]).decode('ascii').split('\0')[0]
        box = BBox(cid, score, left, top, right, bottom, label)
        chunkdata.append(box)

    return chunkdata, frame_id


def decode_chunk_data(data: np.ndarray, chunk: str):
    """
    Decode the input data as a BN chunk data.
    Returns the decoded chunk data. 
    An empty array is returned is data can't be decoded.
    """

    chunk_data = None
    if data is None:
        return chunk_data

    if chunk == 'FeaturePoints':
        chunk_data = []
        kp, offset = decode_chunk_keypoint(data)
        if kp is not None:
            chunk_data.append(kp)

            if offset > 0:
                data = data[offset:]
                kp2, _ = decode_chunk_keypoint(data)
                if kp2 is not None:
                    chunk_data.append(kp2)

    elif chunk == 'FeatureDescriptors':
        chunk_data = []
        descr, offset = decode_chunk_descriptor(data)
        if descr is not None:
            chunk_data.append(descr)
            if offset > 0:
                data = data[offset:]
                descr2, _ = decode_chunk_descriptor(data)
                if descr2 is not None:
                    chunk_data.append(descr2)

    elif chunk == 'BoundingBoxes':
        chunk_data, _ = decode_chunk_bbox(data)

    return chunk_data


def get_chunkdata_by_id(rawdata: np.ndarray, chunk_id: int = 0):
    """
    In case of multipart transmission, returns the buffer attached to each ID.
    """
    chunk_data = []
    if rawdata is None or len(rawdata) == 0 or chunk_id < 0:
        return chunk_data

    pos = len(rawdata) - 4
    while pos >= 0:
        chunk_len = int.from_bytes(rawdata[pos:(pos + 4)], 'big')  # transmitted as big-endian
        if chunk_len > 0 and (pos - 4 - chunk_len) > 0:
            pos -= 4
            chunk_id = int.from_bytes(rawdata[pos:(pos + 4)], 'big')  # transmitted as big-endian   

            pos -= chunk_len
            if chunk_id == chunk_id:
                chunk_data = rawdata[
                             pos:(pos + chunk_len)]  # transmitted as little-endian
                break

        pos -= 4

    return chunk_data


def decode_chunk(device: eb.PvDeviceGEV, buffer: eb.PvBuffer, chunk: str):
    """
    Decode the chunk data attached to the input buffer.
    Decoding happens only if the chunk corresponds to the requested chunk
    """
    rawdata = None
    payload = buffer.GetPayloadType()

    chunk_id = read_chunk_id(device=device, chunk_name=chunk)
    if payload == eb.PvPayloadTypeImage:
        if has_chunk_data(buffer=buffer, chunk_id=chunk_id):
            rawdata = buffer.GetChunkRawDataByID(chunk_id)

    elif payload == eb.PvPayloadTypeMultiPart:
        if buffer.GetMultiPartContainer().GetPartCount() == 3:
            chkbuffer = buffer.GetMultiPartContainer().GetPart(2).GetChunkData()
            if chkbuffer.HasChunks():
                dataptr = buffer.GetMultiPartContainer().GetPart(2).GetDataPointer()
                rawdata = get_chunkdata_by_id(rawdata=dataptr, chunk_id=chunk_id)

    chunk_data = decode_chunk_data(data=rawdata, chunk=chunk)

    return chunk_data


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
                        print(f"Descriptors: fid: {descr.fid} {descr.size} len: {descr.nbits} data: {descr.data[10]}")

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
                        print(f"Descriptors: fid: {descr.fid} {descr.size} len: {descr.nbits} data: {descr.data[0]}")

                    bboxes = decode_chunk(device=device, buffer=pvbuffer, chunk='BoundingBoxes')
                    if bboxes is not None:
                        print(f"BBoxes: {len(bboxes)} data: {bboxes[0]}")

                    # Bottlenose sends as YUV422
                    if image0.GetPixelType() == eb.PvPixelYUV422_8:
                        image_data0 = cv2.cvtColor(image_data0, cv2.COLOR_YUV2BGR_YUY2)
                        image_data1 = cv2.cvtColor(image_data1, cv2.COLOR_YUV2BGR_YUY2)
                        display_image = True

                    if nframes is None:
                        if display_image:
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

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
__author__ = "G. M. Tchamgoue <martin@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

import eBUS as eb
import numpy as np
from collections import namedtuple

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

    fields = ['fid', 'nbits', 'nbytes', 'num', 'data']
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

def decode_chunk_matches(data):
    """
    decode the input buffer as matched keypoints.
    the layout tells the structure of the data
    [0,1] = point16, [2,3] point16x8
    0: COORDINATE_ONLY, 1: INDEX_ONLY
    2: COORDINATE_DETAILED, 3: INDEX_DETAILED
    """
    if len(data) == 0:
        return None

    match_fields = ['layout', 'unmatched', 'points']
    Matches = namedtuple('Matches', match_fields)

    count = int.from_bytes(data[0:4], 'little')
    layout = int.from_bytes(data[4:8], 'little')
    unmatched = int.from_bytes(data[8:12], 'little')
    points = []

    if layout < 2:
        point_fields = ['x', 'y']
        Point = namedtuple('Point', point_fields)
        for i in range(12, 12 + count * 4, 4):
            x = int.from_bytes(data[i:i + 2], 'little')
            y = int.from_bytes(data[i + 2:i + 4], 'little')
            pt = Point(x, y)
            points.append(pt)
    else:
        point_fields = ['x', 'y', 'x2', 'y2', 'd2', 'd1', 'n2', 'n1']
        PointDetailed = namedtuple('PointDetailed', point_fields)
        for i in range(12, 12 + count * 16, 16):
            x = int.from_bytes(data[i:i + 2], 'little')
            y = int.from_bytes(data[i + 2:i + 4], 'little')
            x2 = int.from_bytes(data[i + 4:i + 6], 'little')
            y2 = int.from_bytes(data[i + 6:i + 8], 'little')
            d2 = int.from_bytes(data[i + 8:i + 10], 'little')
            d1 = int.from_bytes(data[i + 10:i + 12], 'little')
            n2 = int.from_bytes(data[i + 12:i + 14], 'little')
            n1 = int.from_bytes(data[i + 14:i + 16], 'little')
            pt = PointDetailed(x, y, x2, y2, d2, d1, n2, n1)
            points.append(pt)

    chunkdata = Matches(layout, unmatched, points)

    return chunkdata

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

    elif chunk == 'FeatureMatches':
        chunk_data = decode_chunk_matches(data)

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
            chkid = int.from_bytes(rawdata[pos:(pos + 4)], 'big')  # transmitted as big-endian

            pos -= chunk_len
            if chkid == chunk_id:
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


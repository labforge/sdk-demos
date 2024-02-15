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
__copyright__ = "Copyright 2024, Labforge Inc."

import ftplib
import ipaddress
from os import path
import time
import sys
import warnings

import eBUS as eb
import cv2

# reference common utility files
sys.path.insert(1, '../common')
from chunk_parser import decode_chunk
from connection import init_bottlenose, deinit_bottlenose

import draw_chunkdata as chk


def handle_buffer(pvbuffer, device):
    payload_type = pvbuffer.GetPayloadType()
    if payload_type == eb.PvPayloadTypeImage:
        image = pvbuffer.GetImage()
        image_data = image.GetDataPointer()

        bounding_boxes = decode_chunk(device=device, buffer=pvbuffer, chunk='BoundingBoxes')

        # Bottlenose sends as YUV422
        if image.GetPixelType() == eb.PvPixelYUV422_8:
            image_data = cv2.cvtColor(image_data, cv2.COLOR_YUV2BGR_YUY2)
            # Draw any bounding boxes found
            if bounding_boxes is not None:
                chk.draw_bounding_boxes(image_data, bounding_boxes)
            cv2.imshow("Detections", image_data)


def upload_weights(device, file_name):
    """
    Upload weights file to Bottlenose.
    :param device: The device to upload weights to
    :param file_name: The file to upload (do not unpack the .tar file)
    """
    if not path.exists(file_name):
        raise RuntimeError(f"Unable to find weights file {file_name}")

    # Get device parameters need to control streaming
    device_params = device.GetParameters()
    weights_update = device_params.Get("EnableWeightsUpdate")
    weights_status = device_params.Get("DNNStatus")
    current_ip = device_params.Get("GevCurrentIPAddress")
    if weights_update is None or weights_status is None:
        raise RuntimeError("Unable to find weights update or status parameters, please update your firmware")

    res, ip_address = current_ip.GetValue()
    if res.IsOK():
        ip_address = str(ipaddress.IPv4Address(ip_address))
    else:
        raise RuntimeError("Unable to find current IP address")

    # Enable weights update
    res, val = weights_update.GetValue()
    if val:
        weights_update.SetValue(False)
        print('Disabling weights update')

    weights_update.SetValue(True)
    while True:
        time.sleep(0.1)
        res, status = weights_status.GetValue()
        print(status)
        if res.IsOK():
            if status.find('FTP running') >= 0:
                break
        else:
            raise RuntimeError(f"Unable to update weights, status: {status}")

    # Upload weights file
    ftp = ftplib.FTP(ip_address)
    ftp.login()
    try:
        ftp.storbinary(f"STORE {path.basename(file_name)}", fp=open(file_name, "rb"))
    except Exception as e:
        # Bottlenose validates the uploaded file and will error out if the format is corrupted
        raise RuntimeError(f"Unable to upload weights file: {e}, possibly corrupted file")
    finally:
        ftp.close()

    valid = False
    for _ in range(100):
        res, status = weights_status.GetValue()
        if res.IsOK():
            if status.find('Loaded') >= 0:
                valid = True
                break
        time.sleep(0.1)
    if not valid:
        raise RuntimeError(f"Unable to load weights, status: {status}")

    # Disable weights update
    weights_update.SetValue(False)


def enable_bounding_boxes(device):
    """
    Enable the bounding points chunk data
    :param device: The device to enable bounding box streaming for
    """
    # Get device parameters
    device_params = device.GetParameters()

    # Enable keypoint detection and streaming
    chunkMode = device_params.Get("ChunkModeActive")
    chunkMode.SetValue(True)
    chunkSelector = device_params.Get("ChunkSelector")
    chunkSelector.SetValue("BoundingBoxes")
    chunkEnable = device_params.Get("ChunkEnable")
    chunkEnable.SetValue(True)


def configure_model(device, confidence=0.2):
    """
    Configure the AI model
    :param device: device
    :param confidence: Confidence threshold for detections
    """
    dnn_enable = device.GetParameters().Get("DNNEnable")
    dnn_confidence = device.GetParameters().Get("DNNConfidence")
    dnn_debug = device.GetParameters().Get("DNNDrawOnStream")

    if dnn_debug is None or dnn_enable is None or dnn_confidence is None:
        raise RuntimeError("Unable to find DNN debug parameter, please update your firmware")

    # Please use eBusPlayer to find the best parameter of your model
    dnn_enable.SetValue(True)
    dnn_confidence.SetValue(confidence)

    # Set this parameter to true for Bottlenose draw bounding boxes on the stream before transmitting it out
    dnn_debug.SetValue(False)


def run_demo(device, stream, weights_file):
    """
    Run the demo
    :param device: The device to stream from
    :param stream: The stream to use for streaming
    :param weights_file: The path to the AI model weights file to upload.
    """
    # Get device parameters need to control streaming
    device_params = device.GetParameters()

    # Map the GenICam AcquisitionStart and AcquisitionStop commands
    start = device_params.Get("AcquisitionStart")
    stop = device_params.Get("AcquisitionStop")

    # Enable keypoint detection and streaming
    if weights_file is not None:
        upload_weights(device, weights_file)

    # Enable chunk data transfer for bounding boxes
    enable_bounding_boxes(device)

    # Configure the parameters
    configure_model(device)

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
    weights_file = None
    if len(sys.argv) >= 2:
        weights_file = sys.argv[1]
    if len(sys.argv) >= 3:
        mac_address = sys.argv[2]

    device, stream, buffers = init_bottlenose(mac_address)
    if device is not None:
        run_demo(device, stream, weights_file)

    deinit_bottlenose(device, stream, buffers)

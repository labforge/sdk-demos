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
import csv
sys.path.insert(1, '../common')
from connection import init_bottlenose, deinit_bottlenose


def to_visibility(num):
    if num == 0:
        return "Beginner"
    elif num == 1:
        return "Expert"
    elif num == 2:
        return "Guru"
    elif num == 3:
        return "Invisible"
    else:
        return "Undefined"


def to_csv_file(params, csv_file):
    param_data = []
    for param in params:
        if param is not None:
            _, name = param.GetName()
            _, dname = param.GetDisplayName()
            _, tooltip = param.GetToolTip()
            _, description = param.GetDescription()
            _, category = param.GetCategory()
            _, visibility = param.GetVisibility()
            visibility = to_visibility(visibility)

            # Store parameter values in the dictionary
            if visibility != "Undefined" and visibility != "Invisible":
                param_data.append((category, name, dname, description, tooltip, visibility))
        else:
            break

    fieldnames = ['Category', 'Name', 'DisplayName', 'Description', 'ToolTip', 'Visibility']

    with open(csv_file, mode='w', newline='') as file:
        writer = csv.writer(file)

        # Write header
        writer.writerow(fieldnames)

        # Write parameter data
        for row in param_data:
            writer.writerow(row)


def run_demo(device, stream):
    """
    Run the demo
    :param device: The device to stream from
    :param stream: The stream to use for streaming
    """
    # Get device parameters need to control streaming
    device_params = device.GetParameters()
    stream_params = stream.GetParameters()
    to_csv_file(device_params, "device_params.csv")
    to_csv_file(stream_params, "stream_params.csv")


if __name__ == '__main__':
    mac_address = None
    if len(sys.argv) >= 2:
        mac_address = sys.argv[1]

    device, stream, buffers = init_bottlenose(mac_address)
    if device is not None:
        run_demo(device, stream)

    deinit_bottlenose(device, stream, buffers)

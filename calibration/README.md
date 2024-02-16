# Simple Calibration Parameters Example

This Python script shows how to programmatically set calibration parameters from a calibration file.
This example assume the Labforge YAML file, but can be adapted for any other calibration file.

## Setup

Change the following demo parameters to your desired settings in the ```demo.py``` file.

| ***Parameter***               | ***Description***                               |
|-------------------------------|-------------------------------------------------|
| ```-mac```                    | (optional) The MAC address of the camera. Assumes the first available camera if not specified.|
| ```-kfile```                  | A YAML file containing calibration data.        |

## Usage

```bash
python demo.py -m <MAC> -f path/to/calibration.yaml
# -m (optional) mac address of a Bottlenose or connect to the first one available if not specified
# -f calibration file
```

----
Back to [Samples](../README.md)
# Simple Descriptor Sample

This demo shows how to stream descriptors from a Bottlenose camera.

This example assumes that any other settings related to image quality such as `exposure`, `gain`, and `CCM` is properly set. Please use `Stereo Viewer` or `eBusPlayer` to configure the image quality to your liking.

The Python script shows how to:
- enable chunk data transmission for
[FAST](https://en.wikipedia.org/wiki/Features_from_accelerated_segment_test) keypoints.
- set descriptor parameters
- enable chunk data transmission to obtain the descriptors associated with the keypoints 

## Setup

Change the following demo parameters to your desired settings.

| ***Parameter***      | ***Description***                                                                              |
|----------------------|------------------------------------------------------------------------------------------------|
| ```--mac```          | (optional) The MAC address of the camera. Assumes the first available camera if not specified. |
| ```--max_features``` | (optional) Maximum number of keypoints to transmit. Default = 1000                             |
| ```--threshold```    | (optional) Threshold for the FAST algorithm. Default=20                                        |
| ```--nms```          | (optional) Set to use non-maximum suppression with FAST. Default=false                         |
| `--dlen`             | (optional) Select the length of the AKAZE descriptor in bits. default=120                      |
| `--dwin`             | (optional) Select a window size for AKAZE computation. Default=20, for a 20x20 window          |                                                                                     

## Usage

```bash
python demo.py <parameters>
```

----
Back to [Samples](../README.md)
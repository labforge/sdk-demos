# Simple Keypoint Sample

This demo shows how to stream keypoints from a Bottlenose camera.

This example assumes that any other settings related to image quality such as `exposure`, `gain`, and `CCM` is properly set. Please use `Stereo Viewer` or `eBusPlayer` to configure the image quality to your like.

The Python script shows how to programmatically enable chunk data transmission for 
[FAST](https://en.wikipedia.org/wiki/Features_from_accelerated_segment_test) and GFTT key points.

## Setup

Change the following demo parameters to your desired settings in the ```demo.py``` file.

| ***Parameter***               | ***Description***                               |
|-------------------------------|-------------------------------------------------|
| ```max_fast_features```       | Maximum number of keypoints to transmit.        |
| ```fast_threshold```          | Threshold for the Fast9 algorithm.              |
| ```use_non_max_suppression``` | Whether to use non-maximum suppression.         |

## Usage

```bash
python demo.py ?MAC
# MAC     - (optional) mac address of Bottlenose to connect to or first one
```

----
Back to [Samples](../README.md)
# Simple Keypoint Sample

This is a modified version of the streaming demo that shows how to transmit the key points from Bottlenose.

Please use ```eBusPlayer``` first to configure the image quality, gain, and exposure settings.

The Python script shows how to programmatically enable chunk data transmission for 
[Fast9](https://en.wikipedia.org/wiki/Features_from_accelerated_segment_test) key points.

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
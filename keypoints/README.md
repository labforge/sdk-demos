# Simple Keypoint Sample

This demo shows how to stream keypoints from a Bottlenose camera.

This example assumes that any other settings related to image quality such as `exposure`, `gain`, and `CCM` is properly set. Please use `Stereo Viewer` or `eBusPlayer` to configure the image quality to your like.

The Python script shows how to programmatically enable chunk data transmission for 
[FAST](https://en.wikipedia.org/wiki/Features_from_accelerated_segment_test) and GFTT key points.

## Setup

Change the following demo parameters to your desired settings in the ```demo.py``` file.

| ***Parameter***      | ***Description***                                                                              |
|----------------------|------------------------------------------------------------------------------------------------|
| ```--mac```          | (optional) The MAC address of the camera. Assumes the first available camera if not specified. |
| `--corner_type`      | (optional) One of the feature point detector [FAST, GFTT]. Default=FAST                        |
| `--gftt_detector`    | (optional) Underlying detector for GFTT. One of [Harris, MinEigenValue]. Default=Harris        |
| ```--max_features``` | (optional) Maximum number of keypoints to transmit. Default = 1000                             |
| `--quality_level`    | (optional) Quality level for GFTT. Default = 500                                               |
| `--min_distance`     | (optional) Minimum distance between detected points with GFTT. Default = 15                    |
| `--paramk`           | (optional) Free parameter `k` for GFTT. Default = 0                                            |
| ```--threshold```    | (optional) Threshold for the FAST algorithm. Default=20                                        |
| ```--nms```          | (optional) Set to use non-maximum suppression with FAST. Default=false                         |

## Usage

```bash
python demo.py <parameters>
```

----
Back to [Samples](../README.md)
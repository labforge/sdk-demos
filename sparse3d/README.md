# Simple Sparse PointCloud Example

***This demo requires Bottlenose Stereo, it will not work with Bottlenose Mono.***

This example assumes that:
- your camera is properly [calibrated and the calibration parameters uploaded the camera](). Checkout the [calibration example](../calibration/README.md) to see how to upload your parameters.
- all the image quality related settings such as `exposure`, `gain`, and `CCM` are set. Please use `Stereo Viewer` or `eBusPlayer` to configure the image quality to your like.

The Python script shows how to programmatically 
- set keypoint parameters, only [FAST](https://en.wikipedia.org/wiki/Features_from_accelerated_segment_test) is shown, but can be adapted for [GFTT](https://ieeexplore.ieee.org/document/323794). 
- set keypoint matching parameters
- enable chunk data transmission for sparse point cloud

## Output

The script will display the feature points that are detected in the left image that also
have valid 3d correspondences. Each valid frame will yield a `ply` and `png` file that can be viewed with a 3D 
viewer such as [MeshLab](https://www.meshlab.net/). For example,

```
python demo.py --mac <mac> --offsety1 <offset_from_calibration>
```

## Setup

Set the following arguments to the ```demo.py``` file to change demo behavior.

| ***Parameter***        | ***Description***                                                                              |
|------------------------|------------------------------------------------------------------------------------------------|
| ```--mac```            | (optional) The MAC address of the camera. Assumes the first available camera if not specified. |
| ```--max_keypoints```  | (optional) Maximum number of keypoints to detect. Default 100.                                 |
| ```--fast_threshold``` | (optional) Keypoint threshold for the Fast9 algorithm. Default 20.                             |
| ```--match_xoffset```  | (optional) Matcher horizontal search range. Default 0.                                         |
| ```--match_yoffset```  | (optional) Matcher vertical search range. Default 0.                                           |
| ```--offsety1```       | (optional) The Y offset of the right image. Default 440.                                       | 
## Usage

```bash
python demo.py <parameters>
```

----
Back to [Samples](../README.md)
# Simple Disparity Example

***This demo requires Bottlenose Stereo, it will not work with Bottlenose Mono.***

This example assumes that:
- your camera is properly calibrated 
- the calibration parameters are uploaded the camera. Checkout the [calibration example](../calibration/README.md) to see how to upload your parameters.
- all the image quality related settings such as `exposure`, `gain`, and `CCM` are set. Please use `Stereo Viewer` or `eBusPlayer` to configure the image quality to your like.

The Python script shows how to programmatically 
- set camera into disparity mode
- set disparity parameters 
- acquire disparity map
- display disparity map

## Setup

Set the following arguments to the ```demo.py``` file to change demo behavior.

| ***Parameter***      | ***Description***                                                                              |
|----------------------|------------------------------------------------------------------------------------------------|
| ```--mac```          | (optional) The MAC address of the camera. Assumes the first available camera if not specified. |
| ```--offsety1```     | (optional) The Y offset of the right image. Default 440.                                       | 
| ```--image```        | (optional) Decide whether to stream the left image with disparity.                             |
| ```--cost_path```    | (optional) Sets disparity cost path                                                            |
| ```--crosscheck```   | (optional) Sets crosscheck value                                                               |
| ```--subpixel```     | (optional) Set sub-pixel value                                                                 |
| ```--blockwidth```   | (optional) block size width                                                                    |
| ```--blockheight```  | (optional) block size height                                                                   |
| ```--mindisparity``` | (optional) minimum disparity                                                                   |
| ```--numdisparity``` | (optional) number of disparity                                                                 |
| ```--vsearch```      | (optional) Set vertical search                                                                 |
| ```--threshold```    | (optional) set threshold                                                                       |
| ```--uniqueness```   | (optional) set uniqueness ratio                                                                |
| ```--penalty1```     | (optional) set penalty1                                                                        |
| ```--penalty2```     | (optional) set penalty2                                                                        |
## Usage

```bash
python demo.py <parameters>
```

----
Back to [Samples](../README.md)
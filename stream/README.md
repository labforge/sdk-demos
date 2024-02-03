# Simple Streaming Example

This "Hello World" sample shows you how to connect to a Bottlenose camera, 
receive an image stream, stop streaming, and disconnect from the device.

The example also has the capability to display the video stream, and visualize supported
chunk types such as keypoints, neural network bounding boxes, and frame meta information.
To enable such features please use eBusPlayer that is part of the Pleora eBUS SDK and 
configure the desired chunk type.

If the sample is run with a Bottlenose Stereo camera it automatically enables stereo image
acquisition.

```bash
python stream.py ?MAC ?nframes
# MAC     - (optional) mac address of Bottlenose to connect to or first one
# nframes - (optional) number of frames to aquire before exiting (runs headless)
```

----
Back to [Samples](../README.md)
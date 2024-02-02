# Simple Focus Tool 

This is a simple focus tool that uses a [Laplacian](https://en.wikipedia.org/wiki/Discrete_Laplace_operator) filter to 
quantify the strength of edges in the image. The focus value is then displayed in the console and on the image plane.

If the Bottlenose camera is pointed at a flat surface with image features that sits at the desired depth of focus, 
a maximal value of the Laplacian indicates that the camera is in focus.

This is a modified version of the streaming demo that shows how to transmit the key points from Bottlenose.

Please use ```eBusPlayer``` first to configure the image quality, gain, exposure settings, and for the stereo
camera the desired sensor to focus.

## Usage

```bash
python demo.py ?MAC
# MAC     - (optional) mac address of Bottlenose to connect to or first one
```

----
Back to [Samples](../README.md)
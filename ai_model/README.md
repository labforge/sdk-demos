# AI Model Example

This is a modified version of the streaming demo that shows how to detect bounding boxes in a neural network model.

Please use ```eBusPlayer``` first to configure the image quality, gain, and exposure settings.

This demo uses the [YoloV3](https://pjreddie.com/darknet/yolo/) model to detect bounding boxes in the image, please
download the precompiled model for Bottlenose from [here](https://github.com/labforge/models), a good basic example
is ```yolov3_1_416_416_3.tar```.

You can use the 
[Bottlenose utility](https://github.com/labforge/sdk-demos) to download the model to the camera, or provide the path to
the model as commandline parameter to the demo.

The Python script shows how to programmatically enable chunk data transmission for Bounding Box data, and optionally
how to programmatically upload a model to Bottlenose. Note the model files are packaged with meta information, please
provide the Labforge model (.tar) as is for the upload. Bottlenose will validate the file after the upload.

## Setup

Change the following demo parameters to your desired settings in the ```demo.py``` file.

| ***Parameter***               | ***Description***                       |
|-------------------------------|-----------------------------------------|
| ```confidence```              | Confidence to accept a valid detection. |


## Usage

```bash
python demo.py ?ModelFile ?MAC
# ModelFile - (optional) path to model file to upload to Bottlenose
# MAC       - (optional) mac address of Bottlenose to connect to or first one

```

Please [contact us](https://www.labforge.ca/#contact) for training services and custom model porting.

----
Back to [Samples](../README.md)
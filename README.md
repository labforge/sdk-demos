![Labforge logo](doc/img/logo-2-300x212.png)

## Overview

This repository contains the Python SDK samples for [Bottlenose cameras](https://www.labforge.ca/features-bottlenose/).

The samples allow depth and color streaming, and provide snippets to update intrinsic and extrinsic calibration 
information. 

The samples are based on the Pleora SDK version. We distribute a version of the
SDK [here](https://github.com/labforge/bottlenose/releases/) for a variety of
operating systems.

Bottlenose cameras necessary to use these samples library are available for purchase at [Mouser](https://www.mouser.ca/manufacturer/labforge/).

## Environment Setup

Please follow the steps to set up the [Bottlenose camera from our documentation](https://docs.labforge.ca/docs).

Please download and install the **Pleora eBUS SDK** for your target platform.

Please see the ```requirements.txt``` file in each sample directory for the
required python packages to run.

### Microsoft Windows

Note the Python backend for Microsoft Windows systems does not install Python
support by default. Please [install the following wheels](https://packaging.python.org/en/latest/tutorials/installing-packages/)
for your respective Python version.

| ***Python Version*** | ***Matching Wheel***                   |
|----------------------|----------------------------------------|
| 3.6.x (*)            | ebus_python-*-py36-none-win_amd64.whl  |
| 3.7.x                | ebus_python-*-py37-none-win_amd64.whl  |
| 3.8.x                | ebus_python-*-py38-none-win_amd64.whl  |
| 3.8.x                | ebus_python-*-py39-none-win_amd64.whl  |
| 3.10.x               | ebus_python-*-py310-none-win_amd64.whl |

(*) Some of the examples included in this repository use [Pyside6](https://doc.qt.io/qtforpython-6/PySide6/QtWidgets/index.html)
and require a newer version than Python 3.6.

### Ubuntu Linux

We provide support for Ubuntu Linux. To install the SDK including Python packages
please install corresponding Debian package (.dpkg).

```bash
sudo dpkg -i eBUS_SDK_Ubuntu-<Ubuntu version>-*.deb
```

The corresponding libraries will automatically be installed to

```bash
/opt/pleora/ebus_sdk/Ubuntu-<Ubuntu version>/lib/
```

Use this helper script to include the libraries in your environment

```bash
source /opt/pleora/ebus_sdk/Ubuntu-<Ubuntu version>/bin/set_puregev_env.sh
```

## Code Samples

| ***Sample***    | ***Applicable Device(s)*** |
|-----------------|----------------------------|
| Stream          | Mono, Stereo               |
| Stereo          | Stereo                     |
| Keypoints       | Mono, Stereo               |
| ImageProcessing | Mono, Stereo               |
| Utility         | Mono, Stereo               |


## License
This project is licensed under the [Apache License, Version 2.0](LICENSE).
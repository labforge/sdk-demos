# Bottlenose Calibrator

This example shows how to acquire stereo images, enumerate Bottlenose cameras, and acquire stereo images for 
calibration.

For the tools usage refer to [our documentation](https://docs.labforge.ca/docs/software-modules). A pre-compiled
version of the utility is included as part of the [Bottlenose software](https://github.com/labforge/bottlenose/releases).

## Requirements
 * [eBUS SDK 6.3.0 or later](https://github.com/labforge/bottlenose/releases)
   * For Linux SDK please see the [releases in this repository](https://github.com/labforge/sdk-demos/releases) 
 * [Qt5](https://doc.qt.io/qt-5.15/)
 * [OpenCV 4.3 or newer](https://opencv.org/)
 * [Meson Build System](https://mesonbuild.com/)
 * [XXD](https://www.tutorialspoint.com/unix_commands/xxd.htm)
   * [Microsoft Windows](https://sourceforge.net/projects/xxd-for-windows/)  
 * A C++ compiler
   * Build-essentials for Linux (tested with gcc-10 on Ubuntu 20.04, and 22.04)
   * [Microsoft Visual Studio Build tools for Visual Studio 17 or newer](https://aka.ms/vs/17/release/vs_buildtools.exe) 

### Building the Utility in Ubuntu 22.04

 * Install the above dependencies
 * Build utility
```
meson build
ninja -C build
```
 * The executable for the utility can be found in ```build/src/calibrator```

### Building the Utility in Microsoft Windows

 * Install the above dependencies
 * Configure the path of the utilities
 * See this [helper script](win/install_oss.bat) for how to bootstrap the environment for Qt and its dependencies

----
Back to [Home](../README.md)

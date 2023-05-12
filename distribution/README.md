# Bottlenose Application Installer

This example shows a workflow using Python setup tools to generate and
package the [driver](../driver/README.md) and [utility](../utility/README.md) into
a Windows installer using [NSIS](https://nsis.sourceforge.io/).

## Building the Distribution Package (Windows only)

Make sure all [requirements](requirements.txt) are installed, including the eBUS SDK wheel for your Python interpreter.

```
build.bat
```

## Usage
 * To execute the generated installer. 
 
```
build\updater.exe
```

This will install "Labforge Inc" -> "Bottlenose Utilities" into the Windows start menu.

----
Back to [Home](../README.md)

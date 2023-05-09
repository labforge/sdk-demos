# Bottlenose Application Installer

This example shows a workflow using Python setup tools to generate and
package the [driver](../driver/README.md) and [utility](../utility/README.md) into
a Windows installer using [NSIS](https://nsis.sourceforge.io/).

## Building the Distribution Package

```
build.bat
```

## Usage
 * To install the new eBUS driver call the driver executable as follows
 
```
build\driver.exe --install=eBUSUniversalProForEthernet
```

----
Back to [Home](../README.md)

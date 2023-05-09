# Bottlenose Driver Installation

This is a native application for Windows platforms to update the eBUS driver
to the latest version of the installed SDK.

When writing applications for Bottlenose this application can be called from
an installation script on a Windows target platform to provision the required
drivers if they are not already installed by a previous eBUS SDK installation.

## Building the Driver Installer

```
build.bat
```
 * Distribute the generated executable ```build\driver.exe``` along with all 
   dependent link libraries as part of your installer.

## Usage
 * To install the new eBUS driver call the driver executable as follows
 
```
build\driver.exe --install=eBUSUniversalProForEthernet
```

----
Back to [Home](README.md)

# Utility

This sample uses a Python for Qt6 application to demonstrate the transfer
of calibration data, neural network updates, and firmware updates to the sensor.

See [requirements.txt](./requirements.txt) for the required packages. 

```
# Build the resource files (Linux)
./build.sh
# Build the resource files (Windows)
build.bat

# Run the sample application
python utility.py

# Alternatively use the commandline version to update the firmware
python uploader.py -f {filename} -t {firmware,dnn} -i {Bottlenose IP address}
```

----
Back to [Samples](../README.md)
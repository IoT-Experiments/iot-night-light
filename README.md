# Night Light project
Please find all the instruction on [IoT-Experiments website](https://www.iot-experiments.com)

## Hardware
This source code has been developed for an ESP8266.
It has been tested on the WeMos D1 mini.

## Tools
The repository / source code should be used with the PlatformIO IDE, managing dependencies and simplifying the deployment of the source code.

## Configuration
The device configuration is stored into the SPIFFS of the device.
To update it, duplicate the `data/config.defaut.json` file to `data/config.json` and change the values of this file according to your device.

To update the SPIFF of the device, use the following command:
`pio run --target uploadfs`

## Flash the device
Once the configuration updated, flash the device with the following command:
`pio run --target upload`

# Config / Commands
The device subscribes to config and commands topics. You can use those topics to update the device state.

* 'On' state is sent as boolean
* 'Color' is sent as integer.
* 'Brightness' is in percent (0 to 100).

```json
{ "on": true, "color": 16711680, "brightness": 70 }
```

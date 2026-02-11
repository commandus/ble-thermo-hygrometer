# ble-thermo-hygrometer Bluetooth client for a room thermo-hygrometer

## ble-thermo-hygrometer utility

Run ble-thermo-hygrometer in the console:

```shell
ble-thermo-hygrometer
```

First, a list of discovered devices will be displayed - MAC address, name, and RSSI:

```shell
Press Ctrl+Break (or Ctrl+C) to interrupt
Discovered device ed:76:ba:be:8c:05 LT_8C05 -69dBm
...
```

As the device sends data, it will appear in the console (without timestamps):

```shell
ed:76:ba:be:8c:05 LT_8C05 28.6C 10.0%
ed:76:ba:be:8c:05 LT_8C05 28.5C 10.0%
...
```

This program is just an example; use it as a reference.

## How to use the program for your own purposes

Use the example program ble-thermo-hygrometer.cpp or tests/test-read.cpp as a base.

Link the library:

```
libble.lib
```

Include the header files:

```c++
#include "ble-helper.h"
```

Create a BLEHelper object along with a data receiver object:

```c++
BLEHelper b(new MyDiscoverer, nullptr);
```

The class of the data receiver object will be described below.

Start discovering BLE devices and wait about 20-30 seconds:

```c++
b.startDiscovery();
auto found = b.waitDiscover(1, 30);
```

or specify the expected device with a known MAC address (it needs to be specified as a string):

```c++
auto found = b.waitDiscover("ed:76:ba:be:8c:05");
```

Check how many devices were found:

```c++
if (found <= 0)
...
```

### Class of the data receiver object OnDeviceEvent

Create a new class that inherits from OnDeviceEvent.

Override the following methods:

- discoverFirstTime()
- discoverNextTime()
- onData()

All methods can be empty.

In the onData method, the parameter tempx10 gives the temperature value in tenths of a degree.
The parameter hygrox10 gives the humidity value in tenths of a percent.
The parameter unitCode can have the following values:

- 0 - Celsius scale
- 1 - Fahrenheit scale

Example:

```c++
    void onData(DiscoveredDevice &device, uint16_t tempx10, uint16_t hygrox10, uint8_t unitCode) override
    {
        std::cout << macAddress2string(device.addr) << ' ' << device.name
            << ' ' << tempx10 / 10 << '.' << tempx10 % 10 << (unitCode == 1 ? 'F' : 'C')
            << ' ' << hygrox10 / 10 << '.' << hygrox10 % 10 << '%'
            << std::endl;
    }
```

The stopDiscovery() method stops device discovery.

```c++
b.stopDiscovery(10);
```

The only optional parameter sets the timeout for the method to complete in seconds.

## Building

The build is done with CMake for Visual Studio. You can use CLion with Visual Studio installed on your machine.

## Limitations

Currently, there is only a version for Windows 11 (Windows 10) with WinRT.

WinRT requires a compiler with C++17 standard.

## MIT License

See the license in the LICENSE file.

## Tests

It was tested on a device with the trade name Maestro.

![Maestro](doc/device-maestro.png)

This device shows the following services (16-bit UUID):

```
1800
180a
180f
ffe5
fe59
```

The ffe5 service has characteristics:

```
ffe8 notifications
ffe9 write
```

The program subscribes to notifications of the ffe8 characteristic of the ffe5 service.

The notification sends 13 bytes, the first three bytes are the prefix (AA AA A2), and the last byte is the suffix (55), which do not change:

```
AA AA A2 00 06 01 0E 00 64 01 00 70 55
```

All numbers use big-endian byte order.

Bytes 2-3 - payload length (00 06) - 6 bytes.

Bytes 4-5 - temperature value in tenths of a degree (01 0E).

Bytes 6-7 - humidity value in tenths of a percent (00 64).

Byte 8 - unknown (01).

Byte 9 - temperature scale value (00). 0 - Celsius, 1 - Fahrenheit.

## References

- [Rémi Peyronnet. Température Bluetooth LE dans domoticz par reverse engineering et MQTT auto-discovery Home Assistant](https://www.lprp.fr/2022/07/capteur-bluetooth-le-temperature-dans-domoticz-par-reverse-engineering-et-mqtt-auto-discovery-domoticz-et-home-assistant/)
- [BLEConsole](https://github.com/sensboston/BLEConsole)

## License

MIT License (see LICENSE file).

Copyright (C) 2026 Andrei Ivanov. All rights reserved.

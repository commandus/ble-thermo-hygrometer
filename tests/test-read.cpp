/**
 *  ./test-specific-data 53500b1c810141
 */

#include <iostream>
#include <sstream>
#include <conio.h>
#include "ble-helper.h"

class MyDiscoverer : public OnDeviceEvent {
public:
    void discoverFirstTime(DiscoveredDevice &device) override
    {
        std::cout << "Discovered device " << macAddress2string(device.addr) << ' '
            << device.name << ' '  << device.rssi << "dBm" << std::endl;
        if (!discoverer)
            return;
        int r = discoverer->open(&device);
        if (r)
            std::cerr << "Error open device" << std::endl;
    }

    void discoverNextTime(DiscoveredDevice &device) override
    {
        std::cout << macAddress2string(device.addr) << ' '
            << device.name << ' '  << device.rssi << "dBm" << std::endl;
        if (device.deviceState == DS_IDLE) {
            int r = discoverer->open(&device);
            if (r)
                std::cerr << "Error open device" << std::endl;
        }
    }

    void onData(DiscoveredDevice &device, uint16_t tempx10, uint16_t hygrox10, uint8_t unitCode) override
    {
        std::cout << macAddress2string(device.addr) << ' ' << device.name
            << ' ' << tempx10 / 10 << '.' << tempx10 % 10 << (unitCode == 1 ? 'F' : 'C')
            << ' ' << hygrox10 / 10 << '.' << hygrox10 % 10 << '%'
            << std::endl;
    }
};


int main(int argc, char **argv) {
    BLEHelper b(new MyDiscoverer, nullptr);
    b.startDiscovery();
    // b.waitDiscover("ed:76:ba:be:8c:05");
    auto found = b.waitDiscover(1, 30);

    if (found <= 0) {
        std::cerr << "No any thermo-hygrometer found" << std::endl;
        exit(0);
    }

    for (auto &d : b.devices) {
        std::cout << macAddress2string(d.addr) << ' ' << d.rssi << "dBm\n";
    }
    b.stopDiscovery(10);

    std::cout << "Press any key to exit" << std::endl;

    char ch = getch();

    for (auto &d : b.devices) {
        b.close(&d);
    }

    return 0;
}

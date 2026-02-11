/**
 *  ./test-discover
 */

#include <iostream>
#include <sstream>
#include "ble-helper.h"

int main(int argc, char **argv) {
    BLEHelper b;
    b.startDiscovery();
    // b.waitDiscover("ed:76:ba:be:8c:05");
    b.waitDiscover(1, 30);
    for (auto &d : b.devices) {
        std::cout << "Found " << macAddress2string(d.addr) << ' ' << d.rssi << "dBm\n";
    }
    b.stopDiscovery(10);
    return 0;
}

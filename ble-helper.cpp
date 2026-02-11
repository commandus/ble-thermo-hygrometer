#include <iostream>
#include "ble-helper.h"

ReceivedData::ReceivedData(
    void *aData,
    uint8_t aSize
)
    : size(aSize), data((uint8_t*) aData)
{
}

bool ReceivedData::valid() {
    return size == 13 && data[0] == 0xaa && data[1] == 0xaa && data[2] == 0xa2 && data[12] == 0x55;
}

uint16_t ReceivedData::t10() {
    return (data[5] << 8) | data[6];
}

uint16_t ReceivedData::h10() {
    return (data[7] << 8) | data[8];
}

uint8_t ReceivedData::u() {
    return data[10];
}

uint8_t ReceivedData::bat10() {
    return data[9];
}

DiscoveredDevice::DiscoveredDevice()
    : addr(0), dt(std::chrono::system_clock::now()), rssi(0), deviceState(DS_IDLE),
    impl(nullptr)
{

}

DiscoveredDevice::DiscoveredDevice(
    uint64_t aAddr,
    int16_t aRssi,
    const std::string &aName
)
    : addr(aAddr), dt(std::chrono::system_clock::now()), rssi(aRssi), name(aName), deviceState(DS_IDLE),
    impl(nullptr)
{

}

DiscoveredDevice::~DiscoveredDevice()
{
    if (impl) {
        delete impl;
        impl = nullptr;
    }
}

BLEDiscoverer::BLEDiscoverer()
    : discoveryOn(false), onDeviceEvent(nullptr)
{

}

BLEDiscoverer::BLEDiscoverer(
        OnDeviceEvent *aOnDiscover
)
    : discoveryOn(false), onDeviceEvent(aOnDiscover)
{
    if (aOnDiscover) {
        aOnDiscover->discoverer = this;
        aOnDiscover->discoverExtra = nullptr;
    }
}

BLEDiscoverer::BLEDiscoverer(
        OnDeviceEvent *aOnDiscover,
        void *aDiscoverExtra
)
    : discoveryOn(false), onDeviceEvent(aOnDiscover)
{
    if (aOnDiscover) {
        aOnDiscover->discoverer = this;
        aOnDiscover->discoverExtra = aDiscoverExtra;
    }
}

bool BLEDiscoverer::waitDiscover(
    const char *addressString,
    int seconds
) {
    // wait
    std::unique_lock<std::mutex> lock(mutexDiscoveryState);
    uint64_t a = string2macAddress(addressString);
    cvDiscoveryState.wait_for(lock, std::chrono::seconds(seconds), [this, a] {
        bool found = false;
        for (auto &d : devices) {
            if (d.addr == a) {
                found = true;
                break;
            }
        }
        return found;
    });
    return false;
}

int BLEDiscoverer::waitDiscover(
    int deviceCount,
    int seconds
) {
    // wait
    std::unique_lock<std::mutex> lock(mutexDiscoveryState);
    cvDiscoveryState.wait_for(lock, std::chrono::seconds(seconds), [this, deviceCount] {
        return devices.size() >= deviceCount;
    });
    return (int) devices.size();
}

const DiscoveredDevice notFoundDevice;

const DiscoveredDevice& BLEDiscoverer::find(
    uint64_t addr
) {
    auto it = std::find_if(devices.begin(), devices.end(), [addr](const DiscoveredDevice &dev) {
        return dev.addr == addr;
    });
    if (it == devices.end())
        return notFoundDevice;
    return *it;
}

OnDeviceEvent::OnDeviceEvent()
    : discoverer(nullptr)
{

}

OnDeviceEvent::OnDeviceEvent(
    BLEDiscoverer *aDiscoverer
)
    : discoverer(aDiscoverer)
{

}

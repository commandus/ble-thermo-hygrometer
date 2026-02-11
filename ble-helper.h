#ifndef BLE_HELPER_H
#define BLE_HELPER_H

#include <vector>
#include <map>
#include <chrono>
#include <mutex>

typedef std::chrono::time_point<std::chrono::system_clock> DISCOVERED_TIME;

enum DeviceState {
    DS_IDLE = 0,
    DS_RUNNING
};

class BLEDeviceImpl {
};

class DiscoveredDevice {
public:
    /// Bluetooth address ff:ff:<dd>:<dd>:<dd>:<dd>
    uint64_t addr;
    /// last time discovered
    DISCOVERED_TIME dt;
    /// last discovered RSSI in dBm
    int16_t rssi;
    /// device name
    std::string name;
    /// session open or closed
    DeviceState deviceState;
    /// OS specific implementation
    BLEDeviceImpl *impl;

    DiscoveredDevice();
    DiscoveredDevice(uint64_t addr, int16_t rssi, const std::string &name);
    ~DiscoveredDevice();
};

class ReceivedData {
public:
    uint8_t *data;
    uint8_t size;
    explicit ReceivedData(void *data, uint8_t count);
    bool valid();
    uint16_t t10();
    uint16_t h10();
    uint8_t bat10();
    uint8_t u();
};

class BLEDiscoverer;

class OnDeviceEvent {
public:
    BLEDiscoverer* discoverer;
    void *discoverExtra;
    OnDeviceEvent();
    OnDeviceEvent(BLEDiscoverer* discoverer);
    OnDeviceEvent(BLEDiscoverer* discoverer, void *discoverExtra);
    virtual void discoverFirstTime(DiscoveredDevice &device) = 0;
    virtual void discoverNextTime(DiscoveredDevice &device) = 0;
    virtual void onData(DiscoveredDevice &device, uint16_t  tempx10, uint16_t hygrox10, uint8_t unitCode) = 0;
};

class Image2sRgb;

class BLEDiscoverer {
public:
    bool discoveryOn;
    std::mutex mutexDiscoveryState;
    std::condition_variable cvDiscoveryState;

    std::vector<DiscoveredDevice> devices;
    OnDeviceEvent *onDeviceEvent;

    BLEDiscoverer();
    BLEDiscoverer(OnDeviceEvent *onDeviceEvent);
    BLEDiscoverer(OnDeviceEvent *onDeviceEvent, void *discoverExtra);
    virtual int startDiscovery() = 0;
    virtual void stopDiscovery(int seconds = 10) = 0;
    virtual int open(DiscoveredDevice* device) = 0;
    virtual int close(DiscoveredDevice* device) = 0;
    virtual int pair(const DiscoveredDevice *device) = 0;
    virtual int unpair(const DiscoveredDevice *device) = 0;

    const DiscoveredDevice& find(uint64_t addr);
    int waitDiscover(int deviceCount, int seconds = 20);
    bool waitDiscover(const char *addressString, int seconds = 20);
    // index versions
    int openI(int index);
    int closeI(int index);
};
#ifdef _MSC_VER
#include "ble-helper-win.h"
#endif

#endif

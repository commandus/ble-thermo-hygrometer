#ifndef BLE_HELPER_WIN_H
#define BLE_HELPER_WIN_H

#include <winrt/windows.foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.devices.bluetooth.h>
#include <winrt/windows.devices.enumeration.h>
#include <winrt/windows.devices.bluetooth.advertisement.h>
#include <winrt/windows.devices.bluetooth.genericattributeprofile.h>
#include <winrt/windows.storage.streams.h>

#include "ble-helper.h"
#include "ble-string-helper-win.h"

class BLEDeviceImplWin : public BLEDeviceImpl {
public:
    winrt::Windows::Devices::Bluetooth::BluetoothLEDevice dev;
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService service;
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic characteristic[1];
    BLEDeviceImplWin();
};

void setUUID(
    winrt::guid &uuid,
    void *buffer,
    size_t size
);

class BLEHelper : public BLEDiscoverer {
private:
    winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher advWatcher;
    winrt::event_token discoveryToken;
    winrt::event_token stoppedDiscoveryToken;

    std::string getDeviceName(winrt::Windows::Devices::Bluetooth::BluetoothLEDevice *device);
    std::string getDeviceName(uint64_t addr);

    void commandCharacteristicValueChanged(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &sender,
        const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs &valueChangedValue
    );
    void pairingRequestedHandler(
        const winrt::Windows::Devices::Enumeration::DeviceInformationCustomPairing &devicePairing,
        const winrt::Windows::Devices::Enumeration::DevicePairingRequestedEventArgs &eventArgs
    );
public:
    BLEHelper();
    BLEHelper(OnDeviceEvent *onDiscover);
    BLEHelper(OnDeviceEvent *onDiscover, void *discoverExtra);

    virtual ~BLEHelper();
    int startDiscovery() override;
    void stopDiscovery(int seconds) override;
    int open(DiscoveredDevice*) override;
    int close(DiscoveredDevice*) override;
    int pair(const DiscoveredDevice *device) override;
    int unpair(const DiscoveredDevice *device) override;
};

#endif

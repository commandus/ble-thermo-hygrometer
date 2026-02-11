#ifndef BLE_STRING_HELPER_H
#define BLE_STRING_HELPER_H

#include <cinttypes>
#include <string>

#include <winrt/base.h>
#include <winrt/windows.devices.bluetooth.genericattributeprofile.h>

std::string macAddress2string(uint64_t addr);
// check is valid MAC address e.g. 9c:13:9e:a0:b7:5d
bool isMacAddressString(const std::string &str);
uint64_t string2macAddress(const std::string &str, bool *retValid = nullptr);
std::string hex(const std::string &str);
std::string hex(void *buffer, size_t size);

std::string UUIDToString(const winrt::guid &uuid);
std::string hstring2string(const winrt::hstring &value);
std::string characteristic2String(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &characteristic);
std::string characteristicProperties2String(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &characteristic);
std::string devicePairingStatus2string(winrt::Windows::Devices::Enumeration::DevicePairingResultStatus status);
std::string gattCommunicationStatus2string(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus &status);
std::string sessionStatus2string(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSessionStatus status);
std::string asyncStatus2string(const winrt::Windows::Foundation::AsyncStatus status);
std::string currentTimeStamp();
bool isHex(const std::string &value);
std::string hex2string(const std::string &hex);

#endif

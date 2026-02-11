#include <iomanip>
#include <sstream>

#include <winrt/windows.devices.bluetooth.advertisement.h>

#include "ble-string-helper-win.h"

const char ADDR_DELIMITER = ':';
const char UUID_DELIMITER = '-';
const char NUMBER_FILL = '0';

static const char* GATT_ATTR_PROP_NAMES[] {
"Broadcast",
"Read",
"WriteWithoutResponse",
"Write",
"Notify",
"Indicate",
"AuthenticatedSignedWrites",
"ExtendedProperties",
"ReliableWrites",
"WritableAuxiliaries",
};

static const char* gattSessionStatusStrings[] {
    "closed",
    "active"
};

static const char* asyncStatusStrings[] {
    "started",
    "Completed",
    "canceled",
    "error"
};

std::string macAddress2string(
    uint64_t addr
)
{
    std::stringstream ss;
    ss << std::hex << std::setfill(NUMBER_FILL);
    for (int i = 5; i > 0; i--) {
        ss << std::setw(2) << ((addr >> (i * 8)) & 0xff) << ADDR_DELIMITER;
    }
    ss << std::setw(2) << (addr & 0xff);
    return ss.str();
}


std::string hex(
    const std::string &str
) {
    return hex((void *) str.c_str(), str.size());
}

std::string hex(
    void *buffer,
    size_t size
) {
    std::stringstream ss;
    ss << std::hex << std::setfill(NUMBER_FILL);
    for (size_t i = 0; i < size; i++) {
        ss << std::setw(2) << (int) *(((unsigned char*) buffer)  + i);
    }
    return ss.str();
}

std::string characteristicProperties2String(
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &characteristic
)
{
    std::stringstream ss;
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties properties = characteristic.CharacteristicProperties();
    for (int i = 0; i < 10; i++) {
        if ((properties & (winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties) (1 << i)) != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties::None) {
            ss << GATT_ATTR_PROP_NAMES[i] << ' ';
        }
    }
    return ss.str();
}

std::string UUIDToString(
    const winrt::guid &uuid
)
{
    std::stringstream ss;
    ss << std::hex << std::setfill(NUMBER_FILL)
       << std::setw(8) << uuid.Data1 << UUID_DELIMITER
       << std::setw(4) << uuid.Data2 << UUID_DELIMITER
       << std::setw(4) << uuid.Data3 << UUID_DELIMITER
       << std::setw(2) << (int) uuid.Data4[0]
       << std::setw(2) << (int) uuid.Data4[1] << UUID_DELIMITER
       << std::setw(2) << (int) uuid.Data4[2]
       << std::setw(2) << (int) uuid.Data4[3]
       << std::setw(2) << (int) uuid.Data4[4]
       << std::setw(2) << (int) uuid.Data4[5]
       << std::setw(2) << (int) uuid.Data4[6]
       << std::setw(2) << (int) uuid.Data4[7];
    return ss.str();
}

std::string characteristic2String(
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &characteristic
)
{
    auto uuid = characteristic.Uuid();
    return UUIDToString(uuid);
}

static const char* devicePairingResultStatusStrings[] {
    "paired",
    "not ready to pair",
    "not paired",
    "already paired",
    "connection rejected",
    "too many connections",
    "hardware failure",
    "authentication timeout",
    "authentication not allowed",
    "authentication failure",
    "no supported profiles",
    "protection level could not be met",
    "access denied",
    "invalid ceremony data",
    "pairing canceled",
    "operation already in progress",
    "required handler not registered",
    "rejected by handler",
    "remote device has association",
    "failed"
};

std::string devicePairingStatus2string(
    winrt::Windows::Devices::Enumeration::DevicePairingResultStatus status)
{
    return devicePairingResultStatusStrings[(int) status];
}

static const char* gattCommunicationStatusStrings[] {
    "success",
    "unreachable",
    "protocol error",
    "access denied"
};

std::string gattCommunicationStatus2string(
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus &status
) {
    return gattCommunicationStatusStrings[(int) status];
}

std::string hstring2string(const winrt::hstring &value) {
#pragma warning(push)
#pragma warning(disable: 4244)
    return std::string(value.begin(), value.end());
#pragma warning(pop)
}

std::string sessionStatus2string(
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSessionStatus status
) {
    return gattSessionStatusStrings[(int) status];
}

std::string asyncStatus2string(
    const winrt::Windows::Foundation::AsyncStatus status
) {
    return asyncStatusStrings[(int) status];
}

std::string currentTimeStamp()
{
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%FT%T%z");
    return ss.str();
}

uint64_t string2macAddress(
    const std::string &str,
    bool *retValid
) {
    std::string s(str);
    s.erase(std::remove(s.begin(), s.end(), ':'), s.end());
    if (s.size() + 5 != str.size()) {
        // must have 5 ':' delimiters
        if (retValid)
            *retValid = false;
        return ULLONG_MAX;
    }

    uint64_t r = strtoull(s.c_str(), nullptr, 16);
    if (r == ULLONG_MAX) {
        // check is it an error
        if (errno == ERANGE) {
            if (s.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                if (retValid)
                    *retValid = false;
            }
        }
    }
    if (retValid)
        *retValid = true;
    return r;
}

bool isMacAddressString(
    const std::string &str
) {
    bool r;
    string2macAddress(str, &r);
    return r;
}

bool isHex(
        const std::string &value
) {
    return value.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}

static std::string readHex(
    std::istream &s
)
{
    std::stringstream r;
    s >> std::noskipws;
    char c[3] = {0, 0, 0};
    while (s >> c[0]) {
        if (!(s >> c[1]))
            break;
        auto x = (unsigned char) strtol(c, nullptr, 16);
        r << x;
    }
    return r.str();
}

std::string hex2string(
    const std::string &hex
)
{
    std::stringstream ss(hex);
    return readHex(ss);
}

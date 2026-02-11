#include <sstream>
#include <winrt/windows.storage.streams.h>
#include <iostream>

#include "platform.h"
#include "ble-helper-win.h"

static const winrt::guid bluetoothBaseUUID {0, 0, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

void setUUID(
    winrt::guid &uuid,
    void *buffer,
    size_t size
) {
    switch (size) {
        case 2:
            uuid = bluetoothBaseUUID;
            memmove(&uuid.Data1, buffer, 2);
#if IS_BIG_ENDIAN
            uuid.Data1 = SWAP_BYTES_2(uuid.Data1);
#endif
            break;
        case 4:
            uuid = bluetoothBaseUUID;
            memmove(&uuid.Data1, buffer, 2);
#if IS_BIG_ENDIAN
            uuid.Data1 = SWAP_BYTES_4(uuid.Data1);
#endif
            break;
        case 16: {
            char *c = (char *) buffer;
            memmove(&uuid.Data1, c + 12, 4);
            memmove(&uuid.Data2, c + 10, 2);
            memmove(&uuid.Data3, c + 8, 2);
            memmove(&uuid.Data4, c, 8);
#if IS_BIG_ENDIAN
            uuid.Data1 = SWAP_BYTES_4(uuid.Data1);
            uuid.Data2 = SWAP_BYTES_2(uuid.Data2);
            uuid.Data3 = SWAP_BYTES_2(uuid.Data3);
#endif
            (*((uint64_t*) &uuid.Data4)) = SWAP_BYTES_8(*((uint64_t*) &uuid.Data4));
        }
            break;
        default:
            break;
    }
}

// BLE GATT service ffe5
static const winrt::guid serviceUUID {0x0000fee7, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

static const winrt::guid thermoHygrometerserviceUUID {0x0000ffe5, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb } };

// BLE GATT Characteristics
static const winrt::guid BLEPropertyUUID[] {
    // Notification characteristic ffe8
    { 0x0000ffe8, 0x000000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb }}
};

BLEDeviceImplWin::BLEDeviceImplWin()
    : dev(nullptr), service(nullptr), characteristic { nullptr }
{

}

static void winInit()
{
    winrt::init_apartment();
    // Set console code page to UTF-8 so console known how to interpret string data
    SetConsoleOutputCP(CP_UTF8);
    // Enable buffering to prevent VS from chopping up UTF-8 byte sequences
    setvbuf(stdout, nullptr, _IOFBF, 1000);
}

BLEHelper::BLEHelper()
    : BLEDiscoverer()
{
    winInit();
}

BLEHelper::BLEHelper(OnDeviceEvent *onDiscover)
    : BLEDiscoverer(onDiscover)
{
    winInit();
}

BLEHelper::BLEHelper(
    OnDeviceEvent *onDiscover,
    void *discoverExtra
)
    : BLEDiscoverer(onDiscover, discoverExtra)
{
    winInit();
}

BLEHelper::~BLEHelper() {
    winrt::uninit_apartment();
}

/**
 * Read device name by MAC address from 0x2a00 characteristic. If no 0x2a00 characteristic provided return empty string
 * @param addr BLE device pointer
 * @return device name
 */
std::string BLEHelper::getDeviceName(
    winrt::Windows::Devices::Bluetooth::BluetoothLEDevice *device
) {
    // get service
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult gapServicesResult = device->GetGattServicesForUuidAsync(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceUuids::GenericAccess(), winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
    if (gapServicesResult.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
        return "";
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> gapServices = gapServicesResult.Services();
    if (gapServices.Size() == 0)
        return "";
    // get device name characteristic, number 00002a00 (UUID: 00002a00-0000-1000-8000-00805f9b34fb)
    auto s0 = gapServices.GetAt(0);
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> gapDeviceNameChrs = s0.GetCharacteristics(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicUuids::GapDeviceName());
    if (gapDeviceNameChrs.Size() == 0)
        return "";
    auto readRes = gapDeviceNameChrs.GetAt(0).ReadValueAsync().get();
    if (readRes.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
        return "";
    // read attribute
    winrt::Windows::Storage::Streams::DataReader reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(readRes.Value());
    return hstring2string(reader.ReadString(reader.UnconsumedBufferLength()).c_str());
}

/**
 * Read device name by MAC address from 0x2a00 characteristic. If no 0x2a00 characteristic provided return empty string
 * @param addr MAC address
 * @return device name
 */
std::string BLEHelper::getDeviceName(
    uint64_t addr
) {
    winrt::Windows::Devices::Bluetooth::BluetoothLEDevice dev = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(addr).get();
    if (dev == nullptr)
        return "";
    return getDeviceName(&dev);
}


int BLEHelper::startDiscovery() {
    if (discoveryOn)
        return 0;

    // indicate discovery off
    std::unique_lock<std::mutex> lck(mutexDiscoveryState);
    discoveryOn = true;
    lck.unlock();

    discoveryToken = advWatcher.Received([this](
        const winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher &watcher,
        const winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs &eventArgs
    ) {
        uint64_t addr = eventArgs.BluetoothAddress();
        // Bluetooth Base UUID
        std::vector<winrt::guid> serviceUuids;
        std::string deviceName;
        auto rssi = eventArgs.RawSignalStrengthInDBm();
        for (winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataSection dataSection : eventArgs.Advertisement().DataSections()) {
            unsigned char *c = dataSection.Data().data();
            uint32_t len = dataSection.Data().Length();
            if ((dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::CompleteService128BitUuids())
                || (dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::IncompleteService128BitUuids())){
                size_t sz = len / 16;
                for (int i = 0; i < sz; i++) {
                    winrt::guid g;
                    setUUID(g, c, 16);
                    serviceUuids.push_back(g);
                    c += 16;
                }
            }
            if ((dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::CompleteService32BitUuids())
                || (dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::IncompleteService32BitUuids())){
                size_t sz = len / 4;
                for (int i = 0; i < sz; i++) {
                    winrt::guid g;
                    setUUID(g, c, 4);
                    serviceUuids.push_back(g);
                    c += 4;
                }
            }
            if ((dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::CompleteService16BitUuids())
                || (dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::IncompleteService16BitUuids())) {
                size_t sz = len / 2;
                for (int i = 0; i < sz; i++) {
                    winrt::guid g;
                    setUUID(g, c, 2);
                    serviceUuids.push_back(g);
                    c += 2;
                }
            }
            if (dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::CompleteLocalName()) {
                c = dataSection.Data().data();
                len = dataSection.Data().Length();
                if (!len)
                    return 0;
                deviceName = std::string((const char *) c, len);
            }
            if (dataSection.DataType() == winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementDataTypes::ManufacturerSpecificData()) {
                c = dataSection.Data().data();
                len = dataSection.Data().Length();
                if (len > 0) {
                    //
                }
            }
        }

        // check does service published
        bool found = false;
        for (auto &u : serviceUuids) {
            found = u == serviceUUID;
            if (found)
                break;
        }
        if (!found)
            return 0;

        if (deviceName.empty())
            deviceName = getDeviceName(addr);

        std::unique_lock<std::mutex> lck(mutexDiscoveryState);
        DiscoveredDevice dd = DiscoveredDevice(addr, rssi, deviceName);
        auto f = find(addr);
        if (f.addr == 0) {
            // just discovered
            devices.emplace_back(dd);
            if (onDeviceEvent)
                onDeviceEvent->discoverFirstTime(dd);
        } else {
            if (onDeviceEvent)
                onDeviceEvent->discoverNextTime(dd);
        }
        cvDiscoveryState.notify_all();
        lck.unlock();

        return 0;
    });

    stoppedDiscoveryToken = advWatcher.Stopped([this](const winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher &watcher, const winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStoppedEventArgs &eventArgs) {
        // remove event handlers
        advWatcher.Received(discoveryToken);
        advWatcher.Stopped(stoppedDiscoveryToken);
        std::unique_lock<std::mutex> lck(mutexDiscoveryState);
        // indicate discovery off
        discoveryOn = false;
        lck.unlock();
        cvDiscoveryState.notify_all();
    });

    advWatcher.Start();
    return 0;
}

void BLEHelper::stopDiscovery(
    int seconds
) {
    if (!discoveryOn)
        return;
    // advWatcher.Stopped(notifyToken);
    advWatcher.Stop();
    // wait
    std::unique_lock<std::mutex> lock(mutexDiscoveryState);
    cvDiscoveryState.wait_for(lock, std::chrono::seconds(seconds), [this] {
        return !discoveryOn;
    });
}

void BLEHelper::commandCharacteristicValueChanged(
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic &sender,
    const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs &valueChangedValue
) {
    auto len = valueChangedValue.CharacteristicValue().Length();
    if (!len)
        return;
    uint64_t addr = sender.Service().Device().BluetoothAddress();
    void *b = malloc(len);
    memmove(b, valueChangedValue.CharacteristicValue().data(), len);

    auto d = find(addr);
    if (onDeviceEvent) {
        ReceivedData rd(b, len);
        if (rd.valid())
            onDeviceEvent->onData(d, rd.t10(), rd.h10(), rd.u());
    }
}

void BLEHelper::pairingRequestedHandler(
    const winrt::Windows::Devices::Enumeration::DeviceInformationCustomPairing &devicePairing,
    const winrt::Windows::Devices::Enumeration::DevicePairingRequestedEventArgs &eventArgs
)
{
    switch (eventArgs.PairingKind()) {
        case winrt::Windows::Devices::Enumeration::DevicePairingKinds::ConfirmOnly:
            eventArgs.Accept();
            return;

        case winrt::Windows::Devices::Enumeration::DevicePairingKinds::ProvidePin: {
            std::string pin = "0000";
            winrt::hstring hPin(std::wstring(pin.begin(), pin.end()));
            eventArgs.Accept(hPin);
        }
            return;
        case winrt::Windows::Devices::Enumeration::DevicePairingKinds::ConfirmPinMatch:
            return;
        default:
            break;
    }
}

int BLEHelper::pair(
    const DiscoveredDevice *aDevice
) {
    // get DeviceInformation
    BLEDeviceImplWin *wimpl = (BLEDeviceImplWin *) aDevice->impl;

    auto di = wimpl->dev.DeviceInformation();
    if (!di.Pairing().IsPaired()) {
        if (!di.Pairing().CanPair())
            return -1;
        // pair
        di.Pairing().Custom().PairingRequested({this, &BLEHelper::pairingRequestedHandler});
        di.Pairing().Custom().PairAsync(
            winrt::Windows::Devices::Enumeration::DevicePairingKinds::ConfirmOnly |
            winrt::Windows::Devices::Enumeration::DevicePairingKinds::ProvidePin |
            winrt::Windows::Devices::Enumeration::DevicePairingKinds::ConfirmPinMatch,
            winrt::Windows::Devices::Enumeration::DevicePairingProtectionLevel::None)
            .Completed(
            [this](auto &&op, auto &&status) {
                return 0;
            }
        );
    }
    return 0;
}

int BLEHelper::unpair(
    const DiscoveredDevice *aDevice
) {
    // get DeviceInformation
    BLEDeviceImplWin *wimpl = (BLEDeviceImplWin *) aDevice->impl;
    auto di = wimpl->dev.DeviceInformation();
    if (!di.Pairing().CanPair())
        return -1;
    if (!di.Pairing().IsPaired()) {
        return 0;
    }
    // pair
    di.Pairing().UnpairAsync().Completed(
    [this](auto &&op, auto &&status) {
            return 0;
        });
    return 0;
}

int BLEHelper::open(
    DiscoveredDevice *device
)
{
    BLEDeviceImplWin *wimpl = new BLEDeviceImplWin();
    wimpl->dev = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(device->addr).get();
    // session
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession session = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession::FromDeviceIdAsync(wimpl->dev.BluetoothDeviceId()).get();
    if (session.CanMaintainConnection())
        session.MaintainConnection(true);
    session.SessionStatusChanged([this, device](const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession &session, const winrt::Windows::Foundation::IInspectable &status) {
        device->deviceState = session.SessionStatus() == winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSessionStatus::Active ? DS_RUNNING : DS_IDLE;
    });

    // list services
    /*
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult resultAll = wimpl->dev.GetGattServicesAsync().get();
    for (int i = 0; i < resultAll.Services().Size(); i++) {
        std::cout << "===" << UUIDToString(resultAll.Services().GetAt(i).Uuid()) << std::endl;
    }
    */

    // get service
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult result = wimpl->dev.GetGattServicesForUuidAsync(thermoHygrometerserviceUUID).get();
    if (result.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success) {
        std::cerr << "Error "
        << gattCommunicationStatus2string(result.Status())
        << " getting service " << UUIDToString(thermoHygrometerserviceUUID) << std::endl;
        return -1;
    }

    if (result.Services().Size() < 1)
        return -1;
    wimpl->service = result.Services().GetAt(0);

    // get characteristics
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> services = result.Services();
    bool success = true;
    try {
        for (int i = 0; i < 1; i++) {
            auto characteristics = wimpl->service.GetCharacteristicsForUuidAsync(BLEPropertyUUID[i]).get();
            if (characteristics.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success || characteristics.Characteristics().Size() < 1)
                success = false;
            else
                wimpl->characteristic[i] = characteristics.Characteristics().GetAt(0);
        }
        // notify
        if (success) {
            winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteResult writeResult
                    = wimpl->characteristic[0].WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
                            winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify).get();
            success = (writeResult.Status() ==
                       winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success);
            auto notifyToken = wimpl->characteristic[0].ValueChanged(
                    {this, &BLEHelper::commandCharacteristicValueChanged});
        }
    } catch (winrt::hresult_error const &ex) {
        device->deviceState = DS_IDLE;
        success = false;
    }

    if (success) {
        device->impl = wimpl;
        pair(device);
    } else
        delete wimpl;
    device->deviceState = DS_RUNNING;
    return 0;
}

int BLEHelper::close(
    DiscoveredDevice *device
)
{
    device->deviceState = DS_IDLE;
    BLEDeviceImplWin *wimpl = (BLEDeviceImplWin *) device->impl;
    if (wimpl) {
        try {
            winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteResult writeResult
                = wimpl->characteristic[0].WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
                winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::None).get();
        } catch (winrt::hresult_error const &ex) {
        }
        wimpl->service.Close();
        wimpl->dev.Close();
        // unpair(device);
        delete wimpl;
        device->impl = nullptr;
    }
    return 0;
}

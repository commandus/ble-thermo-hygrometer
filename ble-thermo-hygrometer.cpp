#include <iostream>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <csignal>
#endif

#include "ble-helper.h"

static std::mutex mutexWriteState;
static std::condition_variable cvWriteState;
static bool stopRequest = false;

static void stop()
{
    std::unique_lock<std::mutex> lck(mutexWriteState);
    stopRequest = true;
    cvWriteState.notify_all();
}

#ifndef _MSC_VER
void signalHandler(int signal)
{
    switch (signal) {
        case SIGINT:
            std::cerr << MSG_INTERRUPTED << std::endl;
            stop();
            done();
            std::cerr << MSG_GRACEFULLY_STOPPED << std::endl;
            exit(0);
        case SIGSEGV:
            std::cerr << ERR_SEGMENTATION_FAULT << std::endl;
            printTrace();
            exit(-1);
        case SIGABRT:
            std::cerr << ERR_ABRT << std::endl;
            printTrace();
            exit(-2);
        case SIGHUP:
            std::cerr << ERR_HANGUP_DETECTED << std::endl;
            break;
        case SIGUSR2:	// 12
            std::cerr << MSG_SIG_FLUSH_FILES << std::endl;
            break;
        case 42:	// restart
            std::cerr << MSG_RESTART_REQUEST << std::endl;
            stop();
            done();
            run();
            break;
        default:
            break;
    }
}
#else
BOOL WINAPI winSignalHandler(DWORD signal) {
    std::cerr << "Interrupted.." << std::endl;
    stop();
    return true;
}
#endif

void setSignalHandler()
{
#ifdef _MSC_VER
    SetConsoleCtrlHandler(winSignalHandler,  true);
#else
    struct sigaction action {};
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = &signalHandler;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGUSR2, &action, nullptr);
    sigaction(42, &action, nullptr);
#endif
}

class MyDiscoverer : public OnDeviceEvent {
public:
    void discoverFirstTime(DiscoveredDevice &device) override
    {
        std::cout << "Discovered device " << macAddress2string(device.addr) << ' '
            << device.name << ' '  << device.rssi << "dBm" << std::endl;
        int r = discoverer->open(&device);
        if (r)
            std::cerr << "Error open device" << std::endl;
    }

    void discoverNextTime(DiscoveredDevice &device) override
    {
        if (device.deviceState == DS_IDLE) {
            int r = discoverer->open(&device);
            if (r)
                std::cerr << "Error open device" << std::endl;
        }

        std::cout << macAddress2string(device.addr) << ' '
            << device.name << ' '  << device.rssi << "dBm" << std::endl;
    }

    void onData(DiscoveredDevice &device, uint16_t tempx10, uint16_t hygrox10, uint8_t unitCode) override
    {
        std::cout << macAddress2string(device.addr) << ' ' << device.name
            << ' ' << tempx10 / 10 << '.' << tempx10 % 10 << (unitCode == 1 ? 'F' : 'C')
            << ' ' << hygrox10 / 10 << '.' << hygrox10 % 10 << '%'
            << std::endl;
    }
};

static void run()
{
    BLEHelper b(new MyDiscoverer(), nullptr);
    b.startDiscovery();

    std::cout << "Press Ctrl+Break (or Ctrl+C) to interrupt" << std::endl;
    // filter device somehow
    // b.waitDiscover("ff:ff:92:13:76:14");
    auto devicesFound = b.waitDiscover(1);
    if (!devicesFound)
        exit(0);

    std::unique_lock<std::mutex> lock(mutexWriteState);
    bool *sr = &stopRequest;
    cvWriteState.wait(lock, [&b, sr] {
        return *sr;
    });

    b.stopDiscovery(10);
}

int main(int argc, char **argv) {
    setSignalHandler();
    run();
    return 0;
}

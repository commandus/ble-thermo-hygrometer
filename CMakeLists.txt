#
# mkdir build; cd build; cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
#
cmake_minimum_required(VERSION 3.28)
set(VERSION_INFO 1.0.0)
project(ble-thermo-hygrometer VERSION ${VERSION_INFO})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # WinRT requires C++17 standard
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
    set(OS_SPECIFIC_LIBS Userenv WindowsApp.lib)

    set(BLE_SRC
        ble-helper.cpp
        ble-helper-win.cpp
        ble-string-helper-win.cpp
    )

else()
    set(CMAKE_CXX_STANDARD 11)
    set(BLE_SRC
        ble-helper.cpp
        ble-helper-dbus.cpp
    )
endif()

set(ARGTABLE3_SRC third-party/argtable3/argtable3.c)

add_library(libble STATIC ${BLE_SRC})
target_include_directories(libble PRIVATE "." "third-party" ${VCPKG_INC})

add_executable(ble-thermo-hygrometer ble-thermo-hygrometer.cpp)
target_include_directories(ble-thermo-hygrometer PRIVATE "." "third-party" ${VCPKG_INC})
target_link_libraries(ble-thermo-hygrometer PRIVATE libble ${OS_SPECIFIC_LIBS} )

#
# Tests
#
add_subdirectory(tests)

# Print config
message("")
message(" Windows: must provide VCPKG options like -DCMAKE_TOOLCHAIN_FILE=C:\\git\\vcpkg\\")
message("   scripts\\buildsystems\\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows")
message("")

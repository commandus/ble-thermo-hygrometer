cmake_minimum_required(VERSION 3.28)

project("esl-ble-test")

# WinRT requires C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(TEST_INCS "." ".." "../third-party" ${VCPKG_INC})
set(TEST_LIBS libble ${OS_SPECIFIC_LIBS})

add_executable(test-read test-read.cpp)
target_include_directories(test-read PRIVATE ${TEST_INCS})
target_link_libraries(test-read PRIVATE ${TEST_LIBS})
add_test(NAME test-read COMMAND "test-read")

add_executable(test-discover test-discover.cpp)
target_include_directories(test-discover PRIVATE ${TEST_INCS})
target_link_libraries(test-discover PRIVATE ${TEST_LIBS})
add_test(NAME test-discover COMMAND "test-discover")

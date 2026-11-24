# gs_hardware off Windows (the macOS port): device ids, names and the fake backend as on Windows; the system
# backend lists no devices yet (IOKit for serial and HID, Core Bluetooth for BLE would on macOS).
add_library(gs_hardware STATIC
    include/gs/hardware/Backend.h
    include/gs/hardware/Devices.h
    include/gs/hardware/FakeBackend.h
    src/Devices.cpp
    src/FakeBackend.cpp
    src/posix/NoDevicesBackend.cpp
)
add_library(gs::hardware ALIAS gs_hardware)

target_include_directories(gs_hardware PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_hardware PUBLIC gs::base)
gs_apply_compiler_policy(gs_hardware)

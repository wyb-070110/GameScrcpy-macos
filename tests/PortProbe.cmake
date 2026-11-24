# Port probe: unit tests of the Windows-free modules.
function(gs_add_unit_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;LIBRARIES;ARGUMENTS")
    add_executable(${name} unit/main.cpp ${arg_SOURCES})
    target_link_libraries(${name} PRIVATE gs::doctest ${arg_LIBRARIES})
    gs_apply_compiler_policy(${name})
    add_test(NAME ${name} COMMAND ${name} ${arg_ARGUMENTS})
endfunction()

gs_add_unit_test(gs_base_tests
    SOURCES
        unit/base/BuildInfoTests.cpp
        unit/base/ByteOrderTests.cpp
        unit/base/ClockTests.cpp
        unit/base/ExpectedTests.cpp
        unit/base/InplaceVectorTests.cpp
        unit/base/JsonNestingTests.cpp
        unit/base/LogTests.cpp
        unit/base/SpscQueueTests.cpp
        unit/base/ThreadCheckerTests.cpp
    LIBRARIES
        gs::base
)
target_compile_definitions(gs_base_tests PRIVATE GS_EXPECTED_VERSION="${PROJECT_VERSION}")

gs_add_unit_test(gs_protocol_tests
    SOURCES
        unit/protocol/DatagramTests.cpp
        unit/protocol/DirectMediaTests.cpp
        unit/protocol/GoldenVectorTests.cpp
        unit/protocol/JsonVisitors.h
        unit/protocol/ReedSolomonTests.cpp
        unit/protocol/ReliableTests.cpp
        unit/protocol/RobustnessTests.cpp
    LIBRARIES
        gs::protocol
        gs::nlohmann_json
)
target_compile_definitions(gs_protocol_tests PRIVATE
    GS_PROTOCOL_VECTOR_DIR="${PROJECT_SOURCE_DIR}/protocol/vectors"
)
target_include_directories(gs_protocol_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

gs_add_unit_test(gs_input_tests
    SOURCES
        unit/input/ControlsTests.cpp
        unit/input/DeviceEventsTests.cpp
        unit/input/EngineTests.cpp
        unit/input/ExtensionInputTests.cpp
        unit/input/GeometryTests.cpp
        unit/input/InputSourceTests.cpp
        unit/input/KeyHintsTests.cpp
        unit/input/ModuleTests.cpp
        unit/input/PadProfileTests.cpp
        unit/input/PadReportsTests.cpp
        unit/input/PointerAllocatorTests.cpp
        unit/input/ProfileDiffTests.cpp
        unit/input/ProfilePatchTests.cpp
        unit/input/ProfileTests.cpp
        unit/input/TouchFrameBuilderTests.cpp
        unit/input/TriggerStateTests.cpp
    LIBRARIES
        gs::input
)
target_sources(gs_input_tests PRIVATE unit/input/AcceptanceTests.cpp)

gs_add_unit_test(gs_tsstrip_tests
    SOURCES
        unit/tsstrip/StripTests.cpp
        unit/tsstrip/TokensTests.cpp
    LIBRARIES
        gs::tsstrip
)
target_include_directories(gs_tsstrip_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/tsstrip/src")

gs_add_unit_test(gs_vision_tests
    SOURCES
        unit/vision/BenchmarkTests.cpp
        unit/vision/ColorTests.cpp
        unit/vision/FixtureTests.cpp
        unit/vision/ImageTests.cpp
        unit/vision/MatcherTests.cpp
        unit/vision/TestImages.h
    LIBRARIES
        gs::vision
)
target_compile_definitions(gs_vision_tests PRIVATE
    GS_VISION_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data/vision"
)

# Platform, adb and session: the POSIX backend under src/platform/src/posix. The test fakes speak Winsock,
# which compat/posix maps onto BSD sockets.
gs_add_unit_test(gs_platform_tests
    SOURCES
        unit/platform/CryptoTests.cpp
        unit/platform/UdpSocketTests.cpp
    LIBRARIES
        gs::platform
)

gs_add_unit_test(gs_adb_tests
    SOURCES
        unit/adb/AcceptanceAdbTests.cpp
        unit/adb/AdbClientTests.cpp
        unit/adb/DeviceIdentityTests.cpp
        unit/adb/DeviceListTests.cpp
        unit/adb/DeviceTrackerTests.cpp
        unit/adb/FakeAdbServer.h
        unit/adb/ForwardListTests.cpp
        unit/adb/ServerLauncherTests.cpp
        unit/adb/ShellProtocolTests.cpp
        unit/adb/WirelessServicesTests.cpp
    LIBRARIES
        gs::adb
)
target_include_directories(gs_adb_tests BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix")

gs_add_unit_test(gs_session_tests
    SOURCES
        fakes/FakeDeviceServer.h
        fakes/FakeServerHost.h
        fakes/RecordingListener.h
        unit/adb/FakeAdbServer.h
        unit/session/AcceptanceSessionTests.cpp
        unit/session/AdbServerHostTests.cpp
        unit/session/DatagramVectors.h
        unit/session/DirectHandshakeTests.cpp
        unit/session/DirectTransportTests.cpp
        unit/session/HandshakeTests.cpp
        unit/session/KeyFrameGateTests.cpp
        unit/session/ReceiverStatsTests.cpp
        unit/session/ReliableStreamTests.cpp
        unit/session/SendDelayStatsTests.cpp
        unit/session/SessionTests.cpp
        unit/session/StreamTransportTests.cpp
    LIBRARIES
        gs::session
        gs::nlohmann_json
)
target_include_directories(gs_session_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/session/src"
)
target_compile_definitions(gs_session_tests PRIVATE
    GS_PROTOCOL_VECTOR_DIR="${PROJECT_SOURCE_DIR}/protocol/vectors"
    GS_SERVER_PACKAGE_FILE="${PROJECT_SOURCE_DIR}/android/server/build/outputs/apk/release/server-release-unsigned.apk"
)

# Device tests: they run against a real phone and report "skipped" (77) without GS_ADB_TEST_SERIAL.
add_executable(gs_adb_device_tests integration/adb/AdbDeviceTests.cpp)
target_link_libraries(gs_adb_device_tests PRIVATE gs::adb gs::platform gs::doctest)
target_include_directories(gs_adb_device_tests BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix")
gs_apply_compiler_policy(gs_adb_device_tests)
add_test(NAME gs_adb_device_tests COMMAND gs_adb_device_tests)
set_tests_properties(gs_adb_device_tests PROPERTIES SKIP_RETURN_CODE 77 LABELS device)

add_executable(gs_session_device_tests integration/session/SessionDeviceTests.cpp)
target_link_libraries(gs_session_device_tests PRIVATE gs::session gs::doctest gs::miniz)
target_include_directories(gs_session_device_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(gs_session_device_tests PRIVATE
    GS_DEFAULT_SERVER_APK="${PROJECT_SOURCE_DIR}/android/server/build/outputs/apk/release/server-release-unsigned.apk"
)
gs_apply_compiler_policy(gs_session_device_tests)
add_test(NAME gs_session_device_tests COMMAND gs_session_device_tests)
set_tests_properties(gs_session_device_tests PROPERTIES SKIP_RETURN_CODE 77 LABELS device)

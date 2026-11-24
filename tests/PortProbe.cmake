# Port probe: unit tests of the Windows-free modules.
function(gs_add_unit_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;LIBRARIES")
    add_executable(${name} unit/main.cpp ${arg_SOURCES})
    target_link_libraries(${name} PRIVATE gs::doctest ${arg_LIBRARIES})
    gs_apply_compiler_policy(${name})
    add_test(NAME ${name} COMMAND ${name})
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

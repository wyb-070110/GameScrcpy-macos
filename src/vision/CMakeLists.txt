add_library(gs_vision STATIC
    include/gs/vision/Color.h
    include/gs/vision/Glance.h
    include/gs/vision/Image.h
    include/gs/vision/IntegralImage.h
    include/gs/vision/Matcher.h
    include/gs/vision/Simd.h
    src/Bands.cpp
    src/Bands.h
    src/Color.cpp
    src/Glance.cpp
    src/Image.cpp
    src/IntegralImage.cpp
    src/Kernels.h
    src/KernelsScalar.cpp
    src/Matcher.cpp
    src/Simd.cpp
    src/Template.cpp
)
add_library(gs::vision ALIAS gs_vision)

target_include_directories(gs_vision PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_vision PUBLIC gs::base)
gs_apply_compiler_policy(gs_vision)
# A universal macOS build (CMAKE_OSX_ARCHITECTURES arm64;x86_64) has the AVX2 kernels too, compiled with AVX2 for
# its x86_64 half only; the arm64 half sees an empty file.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|x64" OR "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
    target_sources(gs_vision PRIVATE src/KernelsAvx2.cpp)
endif()
if(MSVC)
    set_source_files_properties(src/KernelsAvx2.cpp PROPERTIES COMPILE_OPTIONS /arch:AVX2)
elseif(APPLE AND "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
    set_source_files_properties(src/KernelsAvx2.cpp PROPERTIES COMPILE_OPTIONS "-Xarch_x86_64;-mavx2")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set_source_files_properties(src/KernelsAvx2.cpp PROPERTIES COMPILE_OPTIONS -mavx2)
endif()

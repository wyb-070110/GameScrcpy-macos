# gs_platform off Windows (macOS port): the POSIX backend under src/posix. Only what the core modules
# below the interface need so far; the rest of the headers stay unimplemented until a module uses them.
if(APPLE AND NOT OPENSSL_ROOT_DIR AND NOT DEFINED ENV{OPENSSL_ROOT_DIR})
    foreach(gs_openssl_prefix IN ITEMS /opt/homebrew/opt/openssl@3 /usr/local/opt/openssl@3)
        if(EXISTS "${gs_openssl_prefix}/include/openssl/evp.h")
            set(OPENSSL_ROOT_DIR "${gs_openssl_prefix}")
            break()
        endif()
    endforeach()
endif()
find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
find_package(Threads REQUIRED)

add_library(gs_platform STATIC
    include/gs/platform/AtomicFile.h
    include/gs/platform/Crypto.h
    include/gs/platform/RunningMarker.h
    include/gs/platform/TcpStream.h
    include/gs/platform/TextReader.h
    include/gs/platform/Thread.h
    include/gs/platform/UdpSocket.h
    src/posix/AtomicFile.cpp
    src/posix/Crypto.cpp
    src/posix/RunningMarker.cpp
    src/posix/TcpStream.cpp
    src/posix/TextJoin.cpp
    src/posix/Thread.cpp
    src/posix/UdpSocket.cpp
    src/posix/UserKey.h
    src/posix/UserProtection.cpp
)
if(APPLE)
    target_sources(gs_platform PRIVATE src/posix/UserKeyKeychain.cpp src/mac/TextReader.mm)
    set_source_files_properties(src/mac/TextReader.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(gs_platform PRIVATE "-framework CoreFoundation" "-framework Security"
                          "-framework Foundation" "-framework CoreGraphics" "-framework Vision")
else()
    target_sources(gs_platform PRIVATE src/posix/UserKeyFile.cpp src/posix/TextReaderNone.cpp)
endif()
add_library(gs::platform ALIAS gs_platform)

target_include_directories(gs_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_platform PUBLIC gs::base PRIVATE OpenSSL::Crypto Threads::Threads)
gs_apply_compiler_policy(gs_platform)

# Off Windows (the macOS port) the media and device libraries come from the system: Homebrew on macOS
# (ffmpeg, sdl3, libusb), found through pkg-config and SDL3's CMake package. The targets keep the names the
# Windows build gives the vendored DLLs. SDL3 and libusb are optional for the libraries that build so far.
find_package(PkgConfig REQUIRED)
pkg_check_modules(gs_ffmpeg REQUIRED IMPORTED_TARGET GLOBAL libavcodec libavformat libavutil libswresample)
foreach(gs_ffmpeg_library IN ITEMS avcodec avformat avutil swresample)
    add_library(FFmpeg::${gs_ffmpeg_library} ALIAS PkgConfig::gs_ffmpeg)
endforeach()
message(STATUS "FFmpeg ${gs_ffmpeg_libavcodec_VERSION} (libavcodec) from ${gs_ffmpeg_libavcodec_PREFIX}")

find_package(SDL3 CONFIG QUIET)
if(TARGET SDL3::SDL3)
    message(STATUS "SDL3 ${SDL3_VERSION}")
    set(GS_HAVE_SDL3 ON CACHE INTERNAL "")
else()
    message(STATUS "SDL3 not found: no sound and no gamepads")
    set(GS_HAVE_SDL3 OFF CACHE INTERNAL "")
endif()

pkg_check_modules(gs_libusb QUIET IMPORTED_TARGET GLOBAL libusb-1.0)
if(TARGET PkgConfig::gs_libusb)
    add_library(libusb::libusb ALIAS PkgConfig::gs_libusb)
endif()

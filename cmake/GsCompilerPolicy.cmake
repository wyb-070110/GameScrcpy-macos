include_guard(GLOBAL)

option(GS_WARNINGS_AS_ERRORS "Treat warnings in first-party code as errors" ON)
option(GS_ENABLE_ASAN "Build with AddressSanitizer" OFF)

if(NOT MSVC)
    include(CheckCXXSourceCompiles)
    set(gs_jthread_probe "#include <stop_token>\n#include <thread>\nint main() { std::jthread worker([](std::stop_token) {}); return 0; }")
    set(CMAKE_REQUIRED_FLAGS "-std=c++20")
    check_cxx_source_compiles("${gs_jthread_probe}" GS_HAS_JTHREAD)
    if(NOT GS_HAS_JTHREAD)
        # libc++ before LLVM 20 (Apple Clang included) keeps jthread and stop_token experimental.
        set(CMAKE_REQUIRED_FLAGS "-std=c++20 -fexperimental-library")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fexperimental-library")
        check_cxx_source_compiles("${gs_jthread_probe}" GS_HAS_EXPERIMENTAL_JTHREAD)
        unset(CMAKE_REQUIRED_LINK_OPTIONS)
        if(NOT GS_HAS_EXPERIMENTAL_JTHREAD)
            message(FATAL_ERROR "The C++ standard library has no std::jthread, even with -fexperimental-library")
        endif()
        add_compile_options($<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-fexperimental-library>)
        add_link_options(-fexperimental-library)
    endif()
    unset(CMAKE_REQUIRED_FLAGS)
endif()

if(GS_ENABLE_ASAN)
    if(NOT MSVC)
        message(FATAL_ERROR "GS_ENABLE_ASAN is only wired up for MSVC")
    endif()
    add_compile_options(/fsanitize=address)
    add_link_options(/INCREMENTAL:NO)
    foreach(lang IN ITEMS C CXX)
        string(REPLACE "/RTC1" "" CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG}")
    endforeach()
endif()

function(gs_apply_compiler_policy target)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        target_compile_features(${target} INTERFACE cxx_std_20)
        return()
    endif()

    target_compile_features(${target} PUBLIC cxx_std_20)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /Zc:preprocessor
            /Zc:inline
            /utf-8
            /external:W0
            /diagnostics:caret
            /wd4702
        )
        if(GS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        target_compile_definitions(${target} PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-missing-field-initializers
            -Wno-trigraphs
            $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
        )
        if(GS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# gs validate checks the processor of each macOS library in a manifest's dylib: an arm64 dylib fits arm64 in both
# forms (one path, or one per processor), and listed for x64 or missing it is caught.
# cmake -DGS=<gs> -DDLL=<an arm64 dylib exporting gs_ext_main> -DWORK=<scratch folder> -P NativeArchitecturesCheck.cmake
foreach(input IN ITEMS GS DLL WORK)
    if("${${input}}" STREQUAL "")
        message(FATAL_ERROR "NativeArchitecturesCheck.cmake needs -D${input}=<value>")
    endif()
endforeach()
file(TO_CMAKE_PATH "${GS}" program)
file(TO_CMAKE_PATH "${DLL}" library)
file(TO_CMAKE_PATH "${WORK}" work)
file(REMOVE_RECURSE "${work}")

function(gs_run name)
    execute_process(COMMAND "${program}" ${ARGN} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE problem)
    set(${name}_result "${result}" PARENT_SCOPE)
    set(${name}_output "${output}" PARENT_SCOPE)
    set(${name}_problem "${problem}" PARENT_SCOPE)
endfunction()

function(gs_folder name dylibs)
    set(folder "${work}/${name}")
    file(MAKE_DIRECTORY "${folder}/lib/arm64" "${folder}/lib/x64")
    file(COPY_FILE "${library}" "${folder}/lib/arm64/clock.dylib")
    file(COPY_FILE "${library}" "${folder}/lib/x64/clock.dylib")
    file(WRITE "${folder}/manifest.json"
         "{\"schemaVersion\": 1, \"id\": \"gstest.architectures\", \"version\": \"1.0.0\", \"gamescrcpy\": \"2.1\", "
         "\"name\": \"Architectures\", "
         "\"license\": \"MIT\", \"requires\": {\"native\": \"^2.0\"}, \"contributes\": {\"native\": {\"dylib\": ${dylibs}}}}")
endfunction()

gs_folder(fits "{\"arm64\": \"lib/arm64/clock.dylib\"}")
gs_run(fits validate "${work}/fits" --json)
if(NOT fits_result EQUAL 0)
    message(FATAL_ERROR "gs validate refused an arm64 dylib listed for arm64 (${fits_result}):\n${fits_output}${fits_problem}")
endif()
string(JSON architecture GET "${fits_output}" libraries 0 architecture)
string(JSON machine GET "${fits_output}" libraries 0 machine)
string(JSON entry GET "${fits_output}" libraries 0 entry)
if(NOT architecture STREQUAL "macos-arm64" OR NOT machine STREQUAL "macos-arm64" OR NOT entry)
    message(FATAL_ERROR "gs validate did not report the arm64 dylib and its gs_ext_main:\n${fits_output}")
endif()

gs_folder(plain "\"lib/arm64/clock.dylib\"")
gs_run(plain validate "${work}/plain")
if(NOT plain_result EQUAL 0 OR NOT plain_output MATCHES "lib/arm64/clock[.]dylib: macos-arm64, exports gs_ext_main")
    message(FATAL_ERROR "gs validate did not check the one dylib of the string form:\n${plain_output}${plain_problem}")
endif()

gs_folder(mixed "{\"arm64\": \"lib/arm64/clock.dylib\", \"x64\": \"lib/x64/clock.dylib\"}")
gs_run(mixed validate "${work}/mixed")
if(mixed_result EQUAL 0 OR
   NOT mixed_output MATCHES "native-machine-mismatch at [$][.]contributes[.]native[.]dylib\\[\"x64\"\\]")
    message(FATAL_ERROR "gs validate took an arm64 dylib listed for x64:\n${mixed_output}${mixed_problem}")
endif()

gs_folder(missing "{\"arm64\": \"lib/arm64/clock.dylib\", \"x64\": \"lib/x64/gone.dylib\"}")
gs_run(missing validate "${work}/missing")
if(missing_result EQUAL 0 OR NOT missing_output MATCHES "file-missing at [$][.]contributes[.]native[.]dylib\\[\"x64\"\\]")
    message(FATAL_ERROR "gs validate did not miss the x64 dylib:\n${missing_output}${missing_problem}")
endif()
message(STATUS "the arm64 dylib fits arm64 in both forms; listed for x64 or missing, it was caught")

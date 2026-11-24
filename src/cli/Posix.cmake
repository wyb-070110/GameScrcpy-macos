# The gs command line tool off Windows (the macOS port): the mock host of gs ext test, as on Windows.
add_library(gs_mockhost STATIC MockHost.cpp MockHost.h)
target_include_directories(gs_mockhost PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gs_mockhost PUBLIC gs::ext gs::nlohmann_json PRIVATE gs::base gs::exthost gs::platform gs::script)
gs_apply_compiler_policy(gs_mockhost)

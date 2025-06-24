include(CMakeFindDependencyMacro)
find_dependency(imgui CONFIG REQUIRED)
find_dependency(plutosvg CONFIG QUIET)

# Compute the installation prefix from the location of this file.
# Assuming the config file is at: <prefix>/lib/cmake/hello_imgui/hello-imguiConfig.cmake
get_filename_component(HELLO_IMGUI_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(HELLO_IMGUI_SHARE_DIR "${HELLO_IMGUI_PREFIX}/share/hello-imgui")

# Now include the helper CMake script from the share directory.
include("${HELLO_IMGUI_SHARE_DIR}/hello_imgui_cmake/hello_imgui_add_app.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/hello-imgui-targets.cmake")

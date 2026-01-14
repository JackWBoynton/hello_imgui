include(hello_imgui_add_app)
hello_imgui_add_app(ecs_probe ecs_probe.main.cpp)

message(STATUS "Fetching tracy via https://github.com/wolfpld/tracy.git @ v0.13.1...")
FetchContent_Declare(tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG       v0.13.1
  GIT_SHALLOW   FALSE
  GIT_PROGRESS  TRUE
  EXCLUDE_FROM_ALL TRUE
  PATCH_COMMAND git apply --check "${CMAKE_CURRENT_LIST_DIR}/tracy_patch_0.patch" && git apply "${CMAKE_CURRENT_LIST_DIR}/tracy_patch_0.patch" || echo "Patches already applied or not needed"
)
FetchContent_MakeAvailable(tracy)

get_target_property(TRACY_SOURCE_DIR Tracy::TracyClient SOURCE_DIR)

target_link_libraries(ecs_probe PUBLIC Tracy::TracyClient)
target_include_directories(ecs_probe PUBLIC
  $<BUILD_INTERFACE:${TRACY_SOURCE_DIR}/public> $<INSTALL_INTERFACE:include>
)

target_include_directories(ecs_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

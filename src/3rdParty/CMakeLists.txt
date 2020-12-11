set(MODULE_NAME 3rdParty)


## Set up 3rdParty library ----------------------------------------------------

add_library(${MODULE_NAME}
  cxxopts.hpp
  Eigen.hpp
  fs_std.hpp
  json/jsoncpp.cpp
  json/json.h
  json/json-forwards.h
  
  openvr/files/compat.h
  openvr/files/Matrices.h
  openvr/files/Matrices.cpp
  openvr/files/Vectors.h
  
  stb_image.h
  tinyfiledialogs/tinyfiledialogs.cpp
  tinyfiledialogs/tinyfiledialogs.h
  tiny_obj_loader.cc
  tiny_obj_loader.h
  ${GL3W_FILES}
  ${OPENVR_FILES}
)

target_include_directories(${MODULE_NAME}
  PUBLIC
    ${GL3W_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

## Adding headers to the public precompiled header.
## NB. For some reason, CMake produces both C and C++ PCH,
##     and the former don't compile. -- CR 2020-02-29
#target_precompile_headers(${MODULE_NAME} PUBLIC
#  <3rdParty/cxxopts.hpp>
#  <3rdParty/fs_std.hpp>
#  <3rdParty/json/json.h>
#  <3rdParty/json/json-forwards.h>
#  <GL/gl3w.h>
#)

set_property(TARGET "3rdParty" PROPERTY FOLDER "3rdParty")


## Set up imgui as a library --------------------------------------------------

find_package(Git QUIET)

message(STATUS "Submodule update for imgui")
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_RESULT)
# execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --remote
#                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                 RESULT_VARIABLE GIT_RESULT)

if(NOT GIT_RESULT EQUAL "0")
  message(FATAL_ERROR "'git submodule update --init' failed with ${GIT_RESULT}, please checkout submodules")
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(imgui
  "${IMGUI_DIR}/imgui/imgui.cpp"
  #"${IMGUI_DIR}/imgui/imgui.h"
  "${IMGUI_DIR}/imgui/imgui_demo.cpp"
  "${IMGUI_DIR}/imgui/imgui_draw.cpp"
  #"${IMGUI_DIR}/imgui/imgui_internal.h"
  "${IMGUI_DIR}/imgui/imgui_widgets.cpp"
  #"${IMGUI_DIR}/imgui/imstb_rectpack.h"
  #"${IMGUI_DIR}/imgui/imstb_textedit.h"
  #"${IMGUI_DIR}/imgui/imstb_truetype.h"
  "${IMGUI_DIR}/imgui/examples/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/imgui/examples/imgui_impl_opengl3.cpp"
)

# Tell imgui to use GL3W as OpenGL loader instad of guessing.
target_compile_definitions("imgui" PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)

target_include_directories("imgui"
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples"
)

target_link_libraries("imgui"
  PUBLIC
    OpenGL::GL
    glfw
)

set_property(TARGET "imgui" PROPERTY FOLDER "3rdParty")

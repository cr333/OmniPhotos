set(MODULE_NAME Utils)

## Source files
file(GLOB sources     "*.cpp")
file(GLOB headers_hpp "*.hpp")
file(GLOB headers_h   "*.h")
list(APPEND headers ${headers_h} ${headers_hpp})

add_library(Utils
  ${sources}
  ${headers}
)

target_link_libraries(Utils
#  Core
  glfw
  glog::glog
)

set_target_properties(${MODULE_NAME} PROPERTIES
  FOLDER Libraries
)

## Include this header first to avoid OpenGL-related compilation errors.
target_precompile_headers(${MODULE_NAME} PUBLIC <GL/gl3w.h>)

## Adding all headers to the public precompiled header.
foreach(header ${headers})
  file(RELATIVE_PATH relative_header ${PROJECT_SOURCE_DIR}/src/ ${header})
  target_precompile_headers(${MODULE_NAME} PUBLIC <${relative_header}>)
endforeach()

target_precompile_headers(${MODULE_NAME} PUBLIC <opencv2/core.hpp>)

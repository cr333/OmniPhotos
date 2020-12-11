set(MODULE_NAME Core)

## Source files of "Base" top level
file(GLOB base_sources "*.cpp")
file(GLOB base_headers "*.hpp")
source_group("General\\Source Files" FILES ${base_sources})
source_group("General\\Header Files" FILES ${base_headers})

## Source files in "/CameraSetup"
file(GLOB cs_sources "CameraSetup/*.cpp")
file(GLOB cs_headers "CameraSetup/*.hpp")
source_group("CameraSetup\\Source Files" FILES ${cs_sources})
source_group("CameraSetup\\Header Files" FILES ${cs_headers})

## Source files in "/Geometry"
file(GLOB geo_sources "Geometry/*.cpp")
file(GLOB geo_headers "Geometry/*.hpp")
source_group("Geometry\\Source Files" FILES ${geo_sources})
source_group("Geometry\\Header Files" FILES ${geo_headers})

## Source files in "/GL"
file(GLOB gl_sources "GL/*.cpp")
file(GLOB gl_headers "GL/*.hpp")
source_group("GL\\Source Files" FILES ${gl_sources})
source_group("GL\\Header Files" FILES ${gl_headers})

## Source files in "/GUI"
file(GLOB gui_sources "GUI/*.cpp")
file(GLOB gui_headers "GUI/*.hpp")

# Exclude VR-related files if not using OpenVR.
if(NOT WITH_OPENVR)
  list(FILTER gui_sources EXCLUDE REGEX ".*VRInterface.cpp$")
  list(FILTER gui_headers EXCLUDE REGEX ".*VRInterface.hpp$")
endif()

source_group("GUI\\Source Files" FILES ${gui_sources})
source_group("GUI\\Header Files" FILES ${gui_headers})

## Source files in "/Loaders"
file(GLOB loaders_sources "Loaders/*.cpp")
file(GLOB loaders_headers "Loaders/*.hpp")
source_group("Loaders\\Source Files" FILES ${loaders_sources})
source_group("Loaders\\Header Files" FILES ${loaders_headers})

## Source files in "/OpticalFlow"
file(GLOB flow_sources "OpticalFlow/*.cpp")
file(GLOB flow_headers "OpticalFlow/*.hpp")
source_group("OpticalFlow\\Source Files" FILES ${flow_sources})
source_group("OpticalFlow\\Header Files" FILES ${flow_headers})

list(APPEND sources ${base_sources} ${cs_sources} ${geo_sources} ${gl_sources} ${gui_sources} ${loaders_sources} ${flow_sources})
list(APPEND headers ${base_headers} ${cs_headers} ${geo_headers} ${gl_headers} ${gui_headers} ${loaders_headers} ${flow_headers})

add_library(${MODULE_NAME}
  ${sources}
  ${headers}
)

target_link_libraries(${MODULE_NAME}
  PUBLIC
    3rdParty
    Utils

    glfw
    OpenGL::GL
    ${OpenCV_LIBS}
)

if(OpenMP_CXX_FOUND)
  target_link_libraries(${MODULE_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()
# NOTE TODO: GLApplication needs to know about VRI render method -> needs to know about imgui. Fix this.
if(WITH_OPENVR)
  target_link_libraries(${MODULE_NAME}
    PUBLIC
      imgui
      ${OPENVR_LIBRARY}
  )

  target_include_directories(${MODULE_NAME}
    PUBLIC
      ${OPENVR_INCLUDE_DIR}
  )

  target_precompile_headers(${MODULE_NAME} PUBLIC <openvr.h>)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
  FOLDER Libraries
)

## Include this header first to avoid OpenGL-related compilation errors.
target_precompile_headers(${MODULE_NAME} PUBLIC
  <GL/gl3w.h>
  <GLFW/glfw3.h>
)

## Remove header that relies on Application* globally defined in main source file.
list(FILTER headers EXCLUDE REGEX ".*GLFW_Callbacks.hpp$")

## Adding all headers to the public precompiled header.
foreach(header ${headers})
  file(RELATIVE_PATH relative_header ${PROJECT_SOURCE_DIR}/src/ ${header})
  target_precompile_headers(${MODULE_NAME} PUBLIC <${relative_header}>)
endforeach()

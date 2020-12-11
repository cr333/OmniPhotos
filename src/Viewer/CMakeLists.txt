set(MODULE_NAME Viewer)

file(GLOB sources "*.cpp")
file(GLOB headers "*.hpp")
list(APPEND sources "${CMAKE_BINARY_DIR}/GitVersion.cpp")
list(APPEND headers "${CMAKE_SOURCE_DIR}/src/GitVersion.hpp")

## Shaders
file(GLOB shaders_general ../Shaders/General/*.glsl)
file(GLOB shaders_include ../Shaders/Include/*.glsl)
source_group("Shaders\\General" FILES ${shaders_general})
source_group("Shaders\\Include" FILES ${shaders_include})
list(APPEND shaders ${shaders_general} ${shaders_include})

add_executable(${MODULE_NAME}
  ${sources}
  ${headers}
  ${shaders}
)

set_target_properties(${MODULE_NAME} PROPERTIES
  FOLDER Runnables
)

target_link_libraries(${MODULE_NAME}
  PUBLIC
    3rdParty
    Core
    Utils

  PRIVATE
    imgui
)

#install (TARGETS ${MODULE_NAME} DESTINATION bin)

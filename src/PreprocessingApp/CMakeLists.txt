set(MODULE_NAME PreprocessingApp)

file(GLOB sources "*.cpp")
file(GLOB headers "*.hpp")
list(APPEND sources "${CMAKE_BINARY_DIR}/GitVersion.cpp")
list(APPEND headers "${CMAKE_SOURCE_DIR}/src/GitVersion.hpp")

# Exclude deformable proxy fitting if not using Ceres.
if(NOT USE_CERES)
  list(FILTER sources EXCLUDE REGEX ".*SphereFitting.cpp$")
  list(FILTER headers EXCLUDE REGEX ".*SphereFitting.hpp$")
endif()


## Project setup --------------------------------------------------------------

if(USE_CERES AND MSVC)
  ## Disable excessive warnings.
  add_compile_options(/wd4244) # Conversion from 'type1' to 'type2', possible loss of data
  add_compile_options(/wd4251) # '***' needs to have dll-interface ...
endif()

add_library(${MODULE_NAME}
  ${sources}
  ${headers}
)

target_link_libraries(${MODULE_NAME}
  3rdParty
  Core
  Utils

  "$<$<BOOL:${USE_CERES}>:ceres>"
)

set_target_properties(${MODULE_NAME} PROPERTIES
  FOLDER Libraries
)

#install (TARGETS ${MODULE_NAME} DESTINATION bin)

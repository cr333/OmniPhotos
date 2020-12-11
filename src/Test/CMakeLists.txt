set(MODULE_NAME Test)

file(GLOB sources "*.cpp")
file(GLOB headers "*.hpp")


#### GTEST ####################################################################

# Source for this: https://github.com/google/googletest/tree/master/googletest
# The reason we are doing it this way is to prevent debug/release library
# errors with the tests.

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(
  ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
  ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
  EXCLUDE_FROM_ALL)

###############################################################################


add_executable(${MODULE_NAME}
  ${sources}
  ${headers}
)

target_include_directories(${MODULE_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${MODULE_NAME}
  3rdParty
  Core
  Utils

  PreprocessingApp

  ${OpenCV_LIBS}
  glog::glog
  gtest_main
  OpenGL::GL
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER Tests)
set_target_properties(gtest PROPERTIES FOLDER Tests)
set_target_properties(gtest_main PROPERTIES FOLDER Tests)

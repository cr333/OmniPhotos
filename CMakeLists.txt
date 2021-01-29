cmake_minimum_required(VERSION 3.0)
project(OmniPhotos)

# Add to module path, so we can find our cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # /lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # /lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # /bin

option(WITH_PCHS "Use precompiled headers" OFF)
if(${CMAKE_VERSION} VERSION_LESS 3.16 AND WITH_PCHS)
    message(WARNING "Precompiled headers are not supported in CMake < 3.16.")
    set(WITH_PCHS OFF)
endif()

if(NOT WITH_PCHS)
  function(target_precompile_headers TARGET SCOPE LIST)
    # Define this function to do nothing
  endfunction()
endif()


#############################
# DEPENDENCIES
#############################

## OpenGL
find_package(OpenGL REQUIRED)

## GL3W (Modern GL Extension Wrangler)
#TODO: get a FindGL3W.cmake
set(GL3W_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/3rdParty/gl3w
  CACHE PATH "GL3W ROOT DIR")

set(GL3W_FILES
  ${GL3W_INCLUDE_DIR}/GL/gl3w.c
  ${GL3W_INCLUDE_DIR}/GL/gl3w.h
  ${GL3W_INCLUDE_DIR}/GL/glcorearb.h
)
include_directories(${GL3W_INCLUDE_DIR})

## GLFW
find_package(glfw3 3.3 REQUIRED)

## Eigen
find_package(Eigen 3.2 REQUIRED)
include_directories(${EIGEN_INCLUDE_DIR})

## OpenCV
find_package(OpenCV 4.1.1 REQUIRED NO_DEFAULT_PATH)
include_directories(${OpenCV_INCLUDE_DIRS})

## OpenMP (optional)
find_package(OpenMP)

## json nlohmann https://github.com/nlohmann/json
set(json_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/3rdParty/json_nlohmann/include)
include_directories(${json_INCLUDE_DIR})

## glog -- Google logging library
## (actually need master newer than Nov 2019)
if(EXISTS glog)
  add_subdirectory(glog)
else() 
  find_package(glog 0.4.0 REQUIRED)
endif()

if(MSVC)
  ## Parallel compiling in Visual Studio and no funny min/max macros.
  add_definitions(/MP /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories("${PROJECT_SOURCE_DIR}/src")

if(APPLE)
  ## Additional link directories (helps with linking to OpenCV on macOS)
  link_directories(/opt/local/lib/)
endif()

## Other third-party code, wrapped in a single library.
add_subdirectory(src/3rdParty)


#############################
# OPTIONS
#############################

option(USE_CERES "Use Ceres in certain apps." OFF)
option(USE_CUDA_IN_OPENCV "Use CUDA in OpenCV." OFF)

## Additional things that are not built by default.
option(WITH_DOCS "Build documentation files." OFF)
option(WITH_OPENVR "Build with OpenVR." ${OPENVR_FOUND})
option(WITH_TEST "Build tests." OFF)
option(WITH_TOOLS "Build with tools." OFF)

if(USE_CERES)
  message(STATUS "Using Ceres.")
  find_package(Ceres REQUIRED)
  add_definitions(-DUSE_CERES)
endif()

if(USE_CUDA_IN_OPENCV)
  message(STATUS "Using CUDA in OpenCV.")
  include_directories(${OpenCV_OPTFLOW_INCLUDE_DIRS}) #cudaOptFlow
  add_definitions(-DUSE_CUDA)
endif()


if(WITH_OPENVR)
  set(OPENVR_ROOT_DIR "src/3rdParty/openvr")
  find_package(OpenVR REQUIRED)
  if(OPENVR_FOUND)
    message(STATUS "With OpenVR.")
    add_definitions(-DWITH_OPENVR)
  endif()
endif()

if(WITH_TOOLS)
  message(STATUS "With Tools.")
  add_definitions(-DWITH_TOOLS)
  add_subdirectory(src/Tools)
endif()


#############################
# CODE GENERATION
#############################

# Generate a file containing the current Git commit hash and version.
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_VERSION)
configure_file("${CMAKE_SOURCE_DIR}/src/GitVersion.cpp.in" "${CMAKE_BINARY_DIR}/GitVersion.cpp" @ONLY)


#############################
# INTERNAL LIBRARIES
#############################

# These are the internal libraries that are needed to run the OmniPhotos
# preprocessing and viewing apps only, and will be tested in the CI pipeline.
add_subdirectory(src/Core)
add_subdirectory(src/Utils)
add_subdirectory(src/PreprocessingApp)


#############################
# DOCUMENTATION
#############################

if(WITH_DOCS)
  add_subdirectory(docs)
endif()


#############################
# EXECUTABLES
#############################

add_subdirectory(src/Preprocessing)
add_subdirectory(src/Viewer)

if(WITH_TEST)
  add_subdirectory(src/Test)
endif()

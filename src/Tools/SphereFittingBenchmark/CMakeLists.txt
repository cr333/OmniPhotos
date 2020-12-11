set(MODULE_NAME SphereFittingBenchmark)

file(GLOB sources "*.cpp")
file(GLOB headers "*.hpp")

add_executable(${MODULE_NAME}
  ${sources}
  ${headers}
)

target_link_libraries(${MODULE_NAME}
  Utils
  PreprocessingApp
)

if(OpenMP_CXX_FOUND)
  target_link_libraries(${MODULE_NAME} OpenMP::OpenMP_CXX)
endif()

add_subdirectory(CompTool)
set_property(TARGET "CompTool" PROPERTY FOLDER "Tools")

if(USE_CERES)
  add_subdirectory(SphereFittingBenchmark)
  set_property(TARGET "SphereFittingBenchmark" PROPERTY FOLDER "Tools")
endif()

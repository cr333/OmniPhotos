set(MODULE_NAME CompTool)

file(GLOB sources     "*.cpp")
file(GLOB headers_hpp "*.hpp")
file(GLOB headers_h   "*.h")
list(APPEND headers ${headers_h} ${headers_hpp})

add_executable(${MODULE_NAME}
  ${sources}
  ${headers}
)

target_link_libraries(${MODULE_NAME}
  3rdParty  # for TCLAP
  Utils
  ${OpenCV_LIBS}
)

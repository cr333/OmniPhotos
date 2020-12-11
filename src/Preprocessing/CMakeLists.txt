set(MODULE_NAME Preprocessing)

add_executable(${MODULE_NAME}
  "Preprocessing-main.cpp")

set_target_properties(${MODULE_NAME} PROPERTIES
  FOLDER Runnables)

target_link_libraries(${MODULE_NAME}
  PRIVATE
    3rdParty
    Core
    Utils
    PreprocessingApp

    ${OpenCV_LIBS}
)

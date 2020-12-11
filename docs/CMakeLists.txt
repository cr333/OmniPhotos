find_package(Doxygen QUIET)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${DOXYGEN_OUTPUT_DIR}/Doxyfile)

# Replace variables inside @@ with the current values.
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create the output directory for us.
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

add_custom_command(
  OUTPUT ${DOXYGEN_INDEX_FILE}
  COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
  WORKING_DIRECTORY ${DOXYGEN_OUTPUT_DIR}
  MAIN_DEPENDENCY Doxyfile
  COMMENT "Generating docs")

add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

file(REMOVE_RECURSE SPHINX_BUILD)

add_custom_target(Sphinx ALL
                  COMMAND ${SPHINX_EXECUTABLE} -E -b html
                  # Tell Breathe where to find the Doxygen output
                  -Dbreathe_projects.OmniPhotos=${DOXYGEN_OUTPUT_DIR}/xml
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")

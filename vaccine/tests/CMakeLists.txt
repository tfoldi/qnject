# add_test(TEST00 ./00_simple_inject.sh)

set(VACCINE_LINKED_UNITTEST_TARGET_NAME "01_linked_unit")
add_executable(${VACCINE_LINKED_UNITTEST_TARGET_NAME}
  01_linked_units.cpp
  http_client.cpp
  unit_utils.cpp
  unit_echo.cpp
  )

set_target_properties(${VACCINE_LINKED_UNITTEST_TARGET_NAME} PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  )

target_include_directories(${VACCINE_LINKED_UNITTEST_TARGET_NAME} PRIVATE "vaccine")
target_link_libraries(${VACCINE_LINKED_UNITTEST_TARGET_NAME} qnject)


# unit tests with dynamically linked libqnject.so/dll/dynlib 
add_test( NAME ${VACCINE_LINKED_UNITTEST_TARGET_NAME}
  COMMAND "${VACCINE_LINKED_UNITTEST_TARGET_NAME}" 
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# generate swagger.json file for validation
add_test(
  NAME 02_generate_swagger_json
  COMMAND qnject_swagger_json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(SWAGGER_CLI)
  add_test(NAME 03_swagger_validation
    COMMAND ${SWAGGER_CLI} validate swagger.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(03_swagger_validation PROPERTIES DEPENDS 02_generate_swagger_json)
  message("adding test 03_swagger_validation")
else()
  message(WARNING "no swagger cli found - skipping swagger.json validation")
endif()

if(WITH_QT)
  add_subdirectory(qt_testapp)

endif()

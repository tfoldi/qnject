# Generate json.h files from json path definitions 
file(GLOB COMPILED_RESOURCES "*.json")
foreach(INPUT_FILE ${COMPILED_RESOURCES})
  get_filename_component(INPUT_FILE_NAME ${INPUT_FILE} NAME NOCACHE)
  set(OUTPUT_FILE ${INPUT_FILE_NAME}.h)
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${RESOURCE_COMPILER} -i ${INPUT_FILE_NAME} ${OUTPUT_FILE}
    DEPENDS ${INPUT_FILE_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling ${INPUT_FILE_NAME} to C header file ${OUTPUT_FILE}")
  list(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
endforeach()

if(WITH_QT)
  set(VACCINE_QT_SOURCES qobject.cpp)
endif()

# On linux/freebsd/osx we are up for a shared library 
add_library(qnject SHARED 
  vaccine.cpp vaccine.h
  echo.cpp 
  utils.hpp 
  lib_preload/dyld_insert.cpp 
  ../deps/mongoose/mongoose.c
  ../deps/mongoose/mongoose.h
  ../deps/loguru/loguru.hpp
  ../deps/json/json.hpp
  ${VACCINE_QT_SOURCES}
  ${COMPILED_RESOURCES}
  )

if(WITH_QT)
  # link with qt libraries 
  target_link_libraries(qnject Qt5::Core Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT})
else(WITH_QT)
  # add libdl (automatically added by qt for qt builds)
  target_link_libraries(qnject ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(WITH_QT)

add_executable(qnject_swagger_json swagger_json.cpp)
target_link_libraries(qnject_swagger_json qnject)

install(TARGETS qnject
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(tests)

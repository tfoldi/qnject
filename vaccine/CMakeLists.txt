# Generate json.h files from json path definitions 
file(GLOB_RECURSE COMPILED_RESOURCES "*.json")
foreach(INPUT_FILE ${COMPILED_RESOURCES})
  get_filename_component(INPUT_FILE_NAME ${INPUT_FILE} NAME CACHE)
  set(OUTPUT_FILE ${INPUT_FILE_NAME}.h)
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${RESOURCE_COMPILER} -i ${INPUT_FILE_NAME} ${OUTPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling ${INPUT_FILE_NAME} to C header file ${OUTPUT_FILE}")
  list(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
endforeach()

# On linux/freebsd/osx we are up for a shared library 
add_library(qnject SHARED 
  vaccine.cpp vaccine.h
  echo.cpp 
  qobject.cpp 
  utils.hpp 
  lib_preload/dyld_insert.cpp 
  ../deps/mongoose/mongoose.c
  ${COMPILED_RESOURCES}
  )

if(Qt5Core_FOUND AND Qt5Widgets_FOUND)
  target_link_libraries(qnject Qt5::Core Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT})
else(Qt5Core_FOUND AND Qt5Widgets_FOUND)
  target_link_libraries(qnject ${CMAKE_THREAD_LIBS_INIT})
endif(Qt5Core_FOUND AND Qt5Widgets_FOUND)

add_subdirectory(tests)

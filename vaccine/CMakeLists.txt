# Generate json.h files from json path definitions
file(GLOB COMPILED_RESOURCES "*.json")
foreach (INPUT_FILE ${COMPILED_RESOURCES})
    get_filename_component(INPUT_FILE_NAME ${INPUT_FILE} NAME NOCACHE)
    set(OUTPUT_FILE ${INPUT_FILE_NAME}.h)
    add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${RESOURCE_COMPILER} -i ${INPUT_FILE_NAME} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Compiling ${INPUT_FILE_NAME} to C header file ${OUTPUT_FILE}")
    list(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
endforeach ()


function(add_vaccine_serum SERUM_NAME PREFIX HANDLER_FQN ROOT_CPP)
  set(PREFIXED "brilliant::route::prefix(\"${PREFIX}\", ${HANDLER_FQN});")
  set(INGREDIENT_HEADER  "${CMAKE_CURRENT_BINARY_DIR}/serum-${SERUM_NAME}.hpp")

  file(GENERATE 
		OUTPUT ${INGREDIENT_HEADER}
		CONTENT "#pragma once\n#include \"${ROOT_CPP}\"\nconst auto ${SERUM_NAME}SerumHandler = ${PREFIXED};"

  )

  add_library( ${SERUM_NAME} ${ROOT_CPP} ${INGREDIENT_HEADER} ${ARGN})

  set_target_properties(${SERUM_NAME} PROPERTIES LINKER_LANGUAGE CXX)


  message("[ SERUM ] Written vaccine serum '${SERUM_NAME} to ${INGREDIENT_HEADER}'")
  message("                                '${PREFIXED}")
endfunction()

## DEPENDENCIES ################################################################

set(BRILLIANT_REQUEST request.h )
set(MONGOOSE ../deps/mongoose/mongoose.c ../deps/mongoose/mongoose.h)
set(LOGURU ../deps/loguru/loguru.hpp)
set(JSON ../deps/json/json.hpp )

set(HANDLERS
        qobject-utils.h
        qwidget-json-helpers.h

        http-handlers/base.h
#        http-handlers/qwidgets-list.h
#        http-handlers/qwidgets-show.h
#        http-handlers/qwidgets-update.h
#        http-handlers/qwidgets-grab-image.h
#        http-handlers/menu-tree-get.h
#        http-handlers/menu-tree-trigger-action.h
#        http-handlers/qwidgets-get-menubar.h
		)

set(VACCINE_COCTAIL_SOURCES
        #vaccine.cpp
        vaccine.h
        utils.hpp
        dyld-hooking.h
        lib_preload/dyld_insert.cpp

        ${HANDLERS}

        ${MONGOOSE} ${LOGURU} ${JSON}
        ${BRILLIANT_REQUEST}

        ${VACCINE_QT_SOURCES}
        ${COMPILED_RESOURCES}
  )




# -----------------------------
function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)

endfunction()


function(add_vaccine_coctail COCTAIL_NAME 
    # add the files as the rest ... HANDLER_FILES
    )

  set(SERUM_HEADERS "#include \"request.h\"\n${ROUTE_ALIASES}\n")
  set(INGREDIENT_WRAPPERS)
  # Add each include
  foreach (INPUT_FILE ${ARGN})
      get_filename_component(INPUT_FILE_NAME ${INPUT_FILE} NAME NOCACHE)
      set(FN "${CMAKE_CURRENT_BINARY_DIR}/serum-${INPUT_FILE_NAME}.hpp")
      list(APPEND SERUM_HEADERS "#include \"${FN}\"\n")
      list(APPEND INGREDIENT_WRAPPERS ${FN})
  endforeach ()

  # Add each handler
  set(VACCINE_COCTAIL_HANDLER)
  foreach (INPUT_FILE ${ARGN})
      get_filename_component(INPUT_FILE_NAME ${INPUT_FILE} NAME NOCACHE)
      list(APPEND VACCINE_COCTAIL_HANDLER "${INPUT_FILE}SerumHandler,")
  endforeach ()

  JOIN("${SERUM_HEADERS}" "\n" HEADERS_JOINED)
  JOIN("${VACCINE_COCTAIL_HANDLER}" "\n    " HANDLERS_JOINED)


  set(METHOD_NOT_FOUND "    qnject::api::method_not_found")
  set(VACCINE_COCTAIL_SRC
    "${HEADERS_JOINED}\n\nconst auto coctailHandler = brilliant::route::prefix(\"api\", brilliant::route::any_of(\n${HANDLERS_JOINED}\n${METHOD_NOT_FOUND}));\n\n#include \"vaccine.cpp\"")

  set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/coctail-recepie-${COCTAIL_NAME}.cpp")
  file(GENERATE OUTPUT ${OUTPUT_FILE} CONTENT "${VACCINE_COCTAIL_SRC}")
  message("[ COCTAIL ] Written vaccine coctail '${COCTAIL_NAME}' prescription to: ${OUTPUT_FILE}}")


  add_library(${COCTAIL_NAME} SHARED ${VACCINE_COCTAIL_SOURCES} ${OUTPUT_FILE} ${INGREDIENT_WRAPPERS})

  # Add each handler
	target_link_libraries(${COCTAIL_NAME} ${ARGN})


endfunction()


  include_directories( ../deps/json ../deps/loguru ../deps/mongoose ) 


if (WITH_QT)
  set(VACCINE_QT_SOURCES qwidget.cpp tableau.cpp)

  add_vaccine_serum(qwidgetIngredient "qwidget"
		serums::qwidgets::qWidgetPath qwidget.cpp
  
        qobject-utils.h
        qwidget-json-helpers.h

        http-handlers/base.h
        http-handlers/qwidgets-list.h
        http-handlers/qwidgets-show.h
        http-handlers/qwidgets-update.h
        http-handlers/qwidgets-grab-image.h
        http-handlers/menu-tree-get.h
        http-handlers/menu-tree-trigger-action.h
        http-handlers/qwidgets-get-menubar.h
		)

  # add_vaccine_serum(tableauIngredient "tableau" tableau.cpp)

  add_vaccine_coctail(qnject qwidgetIngredient 
		# tableauIngredient
		)

  target_link_libraries(qnject ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES})
  include_directories( ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} )
endif ()




# On linux/freebsd/osx we are up for a shared library
#add_library(qnject SHARED ${VACCINE_COCTAIL_SOURCES})




if (MACOSX)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif (MACOSX)

if (WITH_QT)
    # link with qt libraries
    target_link_libraries(qnject Qt5::Core Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT})
else (WITH_QT)
    # add libdl (automatically added by qt for qt builds)
    target_link_libraries(qnject ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif (WITH_QT)

add_executable(qnject_swagger_json swagger_json.cpp)
target_link_libraries(qnject_swagger_json qnject)

install(TARGETS qnject
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)


# TESTS

add_subdirectory(tests)

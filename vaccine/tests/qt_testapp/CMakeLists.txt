# Instruct CMake to run moc nd uic automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON) 

add_executable(qt_testapp 
  main.cpp 
  mainwindow.cpp	mainwindow.h mainwindow.ui 
   unit_qt.cpp 
   ../http_client.cpp
  )

  
  
  
#target_link_libraries(qt_testapp  ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} )
 target_link_libraries(qt_testapp qnject ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} )

add_test(NAME 04_qt_testapp
  COMMAND qt_testapp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_tests_properties(04_qt_testapp
  PROPERTIES ENVIRONMENT "RUN_TESTS=1")



if (WIN32)

    set(MSVC_BUILD_TYPE "Debug" CACHE STRING "Set the build type here for copying the necessary DLLS (as the CMake MSVC generator has no idea at generate time about the build type to be triggered)")
    
    function(add_dlls TARGET_NAME)
        foreach(DLL_NAME ${ARGN})
            message("[DLL] ${DLL_NAME}")
            add_custom_command(
                TARGET ${TARGET_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_NAME}"
                    $<TARGET_FILE_DIR:${TARGET_NAME}>)
        endforeach()
    endfunction(add_dlls)


    function(add_qt_dlls TARGET_NAME)
        foreach(DLL_NAME ${ARGN})
            add_dlls(${TARGET_NAME} "${Qt5Core_DIR}/../../../bin/${DLL_NAME}.dll")
        endforeach()
    endfunction()
    
    add_dlls(qt_testapp "${CMAKE_BINARY_DIR}/vaccine/${MSVC_BUILD_TYPE}/qnject.dll")
    
    if (MSVC_BUILD_TYPE STREQUAL "Release--")
        add_qt_dlls(qt_testapp Qt5Core Qt5Widgets Qt5Gui )
    else()
        add_qt_dlls(qt_testapp Qt5Cored Qt5Widgetsd Qt5Guid icuin53)
    endif()

endif(WIN32)
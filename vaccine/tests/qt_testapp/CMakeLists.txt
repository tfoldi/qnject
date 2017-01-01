# Instruct CMake to run moc nd uic automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON) 

add_executable(qt_testapp 
  main.cpp 
  mainwindow.cpp	mainwindow.h mainwindow.ui 
  unit_qt.cpp 
  ../http_client.cpp
  )
target_link_libraries(qt_testapp Qt5::Widgets qnject)

add_test(NAME 04_qt_testapp
  COMMAND qt_testapp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_tests_properties(04_qt_testapp
  PROPERTIES ENVIRONMENT "RUN_TESTS=1")


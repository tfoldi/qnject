# Instruct CMake to run moc nd uic automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON) 

add_executable(qt_testapp main.cpp mainwindow.cpp	mainwindow.h mainwindow.ui)
target_link_libraries(qt_testapp Qt5::Widgets qnject)

add_test(NAME 04_qt_testapp
  COMMAND qt_testapp 0
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

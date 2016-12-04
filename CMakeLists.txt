cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MACOSX_RPATH 1) 

# Allow double colon in library iports
cmake_policy(SET CMP0028 OLD)

project(qnject)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the Qt package
find_package(Qt5Core)
find_package(Qt5Widgets)

configure_file(config.h.in config.h)

enable_testing()
add_subdirectory(vaccine)

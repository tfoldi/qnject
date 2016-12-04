
add_library(qnject SHARED dyld_insert.cpp)

target_link_libraries(qnject Qt5::Core Qt5::Widgets)


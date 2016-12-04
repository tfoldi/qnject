
add_library(qnject SHARED event_loop.cpp lib_preload/dyld_insert.cpp ../deps/mongoose/mongoose.c)

target_link_libraries(qnject Qt5::Core Qt5::Widgets)


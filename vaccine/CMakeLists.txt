
add_library(qnject SHARED vaccine.cpp echo.cpp qobject.cpp lib_preload/dyld_insert.cpp ../deps/mongoose/mongoose.c)

target_link_libraries(qnject Qt5::Core Qt5::Widgets)

add_subdirectory(tests)

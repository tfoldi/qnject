
add_library(qnject SHARED vaccine.cpp echo.cpp qobject.cpp utils.hpp lib_preload/dyld_insert.cpp ../deps/mongoose/mongoose.c)

if(Qt5Core_FOUND AND Qt5Widgets_FOUND)
  target_link_libraries(qnject Qt5::Core Qt5::Widgets)
else(Qt5Core_FOUND AND Qt5Widgets_FOUND)
  target_link_libraries(qnject)
endif(Qt5Core_FOUND AND Qt5Widgets_FOUND)

add_subdirectory(tests)

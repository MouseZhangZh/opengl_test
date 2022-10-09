include_directories(../include)
include_directories(/opt/homebrew/opt/freetype/include/freetype2)

find_package(Freetype REQUIRED)

set(opengl_test_SRC
        main.cpp
        glad.c)

link_libraries("-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo")

link_directories(../lib-arm64)

add_executable(opengl_test ${opengl_test_SRC})

target_link_libraries(opengl_test glfw3 ${FREETYPE_LIBRARIES})
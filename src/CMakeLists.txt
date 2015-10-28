find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(world world-boost.cpp world.cpp tank.cpp)
add_executable(tank tank-process.cpp)
add_executable(tankclient tankclient.cpp)

target_link_libraries(world ${CURSES_LIBRARIES})

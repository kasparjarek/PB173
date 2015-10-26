find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(world world-boost.cpp world.cpp gameboard.cpp)
add_executable(tank tank.cpp)
add_executable(tankclient tankclient.cpp)

target_link_libraries(world ${CURSES_LIBRARIES})

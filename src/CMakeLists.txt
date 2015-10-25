find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(world world-boost.cpp world.cpp)
add_executable(tank tank.cpp)
add_executable(tankclient tankclient.cpp)
add_executable(gameboard gameboard.cpp)

target_link_libraries(gameboard ${CURSES_LIBRARIES})

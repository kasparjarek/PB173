find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(world world-boost.cpp world.cpp tank.cpp)
add_executable(tankclient tankclient.cpp)
add_executable(worldclient WorldClient.cpp)

target_link_libraries(worldclient ${CURSES_LIBRARIES})

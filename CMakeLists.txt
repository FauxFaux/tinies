cmake_minimum_required(VERSION 3.4)
project(tinies)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -pedantic -Wextra -mavx -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pedantic -Wextra -mavx -fsanitize=address")

foreach(linux btrfs_puncher catfight sponger xwinswitch xlines)
    add_executable(${linux} ${linux}.c)
endforeach(linux)

target_link_libraries(xlines X11)
target_link_libraries(xwinswitch X11)
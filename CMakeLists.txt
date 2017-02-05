cmake_minimum_required(VERSION 3.4)
project(tinies)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -pedantic -Wextra -mavx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pedantic -Wextra -mavx")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

foreach (linux btrfs_puncher drop-privs-harder catfight grant-read safenod sponger xwinswitch xlines)
    add_executable(${linux} ${linux}.c)
endforeach (linux)

# totally half-assed attempt to defer to pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_SECCOMP libseccomp)

target_link_libraries(drop-privs-harder ${PC_SECCOMP_LIBRARIES})

pkg_check_modules(PC_CAP libcap)
target_link_libraries(grant-read ${PC_CAP_LIBRARIES})

pkg_check_modules(PC_X11 x11)
target_link_libraries(xlines ${PC_X11_LIBRARIES})
target_link_libraries(xwinswitch ${PC_X11_LIBRARIES})

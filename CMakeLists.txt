cmake_minimum_required(VERSION 3.16)

project(sanepp)

# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++17)

link_directories(/usr/local/lib/)
include_directories(/usr/include/sane)

file(GLOB LIBSANEPP_SOURCES "src/SaneDev.cpp")

add_library(sanepp STATIC ${LIBSANEPP_SOURCES})
target_include_directories(sanepp PUBLIC include)

file(GLOB SANEPPCMD_SOURCES "src/SaneDevScan.cpp")

add_executable(saneppscan ${SANEPPCMD_SOURCES})

target_link_libraries(saneppscan sanepp sane)

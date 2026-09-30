cmake_minimum_required(VERSION 3.20)
project(goose)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    goose
    src/main.cpp
    src/wal/wal.cpp
)
target_compile_features(goose PRIVATE cxx_std_23)

find_package(ZLIB REQUIRED)
target_link_libraries(goose PRIVATE ZLIB::ZLIB)




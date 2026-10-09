cmake_minimum_required(VERSION 3.20)
project(goose)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    goose
    src/main.cpp
    src/wal/Wal.cpp
    src/dsa/MemTable.cpp
    src/storage/SSTable/Sit.cpp
    src/storage/BackgroundWork.cpp
)

target_compile_features(goose PRIVATE cxx_std_23)
target_compile_options(goose PRIVATE -Wall -Wextra -Werror -Wpedantic)

find_package(ZLIB REQUIRED)
target_link_libraries(goose PRIVATE ZLIB::ZLIB)




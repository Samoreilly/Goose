cmake_minimum_required(VERSION 3.20)
project(goose)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    goose
    src/main.cpp
    src/engine/wal/Wal.cpp
    src/engine/dsa/MemTable.cpp
    src/engine/storage/SSTable/Sit.cpp
    src/engine/storage/BackgroundWork.cpp
    src/engine/storage/Compression/Compress.cpp
    src/engine/storage/Compression/CompressionMethods.cpp
    src/engine/storage/Compaction/Compaction.cpp
    src/engine/wal/Replay.cpp
)

target_compile_features(goose PRIVATE cxx_std_23)
target_compile_options(goose PRIVATE -Wall -Wextra -Werror -Wpedantic)

find_package(ZLIB REQUIRED)
target_link_libraries(goose PRIVATE ZLIB::ZLIB)




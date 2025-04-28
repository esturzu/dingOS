cmake_minimum_required(VERSION 3.15)
project(doomgeneric C)

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

list(FILTER SRC_FILES EXCLUDE REGEX "src/doomgeneric_[a-zA-Z0-9_]+\\.c")
list(FILTER SRC_FILES EXCLUDE REGEX "src/i_(allegro|sdl|emscripten).*\\.c")
list(FILTER SRC_FILES EXCLUDE REGEX "src/i_soso.*\\.c")
list(FILTER SRC_FILES EXCLUDE REGEX "src/i_sosox.*\\.c")

add_library(doomgeneric STATIC ${SRC_FILES})
target_compile_options(doomgeneric PRIVATE
    -mcpu=cortex-a53 -ffreestanding -nostdlib -O2)
target_include_directories(doomgeneric PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

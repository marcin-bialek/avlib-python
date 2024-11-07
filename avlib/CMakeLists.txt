cmake_minimum_required(VERSION 3.16)
project(avlib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswscale
    libavutil
)

execute_process(COMMAND python3 -m pybind11 --includes 
    OUTPUT_VARIABLE PYBIND_INCLUDE_DIRS 
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(PYBIND_INCLUDE_DIRS NATIVE_COMMAND ${PYBIND_INCLUDE_DIRS})
execute_process(COMMAND python3-config --extension-suffix 
    OUTPUT_VARIABLE PYBIND_SUFFIX 
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB_RECURSE SRCS src/*.cc)

add_library(${PROJECT_NAME} SHARED ${SRCS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ${PYBIND_SUFFIX}
)

target_compile_options(${PROJECT_NAME} PRIVATE ${PYBIND_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PkgConfig::LIBAV)

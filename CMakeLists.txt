cmake_minimum_required(VERSION 2.6)
project(monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# try to compile a dummy program that imports filesystem to check for filesystem module support.
# Solution based on https://stackoverflow.com/questions/54290254/problem-adding-stdfilesystem-to-cmake-project
try_compile(filesystem_is_supported "${CMAKE_BINARY_DIR}/temp" 
    "${CMAKE_SOURCE_DIR}/extra/fs_test.cpp"
    CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
)

# fmt is used for the format specification support
add_subdirectory(third_party/fmt)
# backward-cpp is used to provide stack trace under program failures.
add_subdirectory(third_party/backward-cpp)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(monitor ${SOURCES} ${BACKWARD_ENABLE})

if (filesystem_is_supported)
    message(STATUS "Using <filesystem>")
    target_link_libraries(monitor ${CURSES_LIBRARIES} fmt::fmt)
else()
    message(STATUS "Using <experimental/filesystem>")
    add_compile_definitions(USE_EXPERIMENTAL_FILESYSTEM)
    # Add the experimental filesystem library to compile with older versions of gcc and clang
    target_link_libraries(monitor ${CURSES_LIBRARIES} fmt::fmt stdc++fs)
endif()

target_compile_options(monitor PRIVATE -Wall -Wextra -Werror)
add_backward(monitor)

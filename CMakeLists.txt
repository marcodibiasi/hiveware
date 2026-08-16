cmake_minimum_required(VERSION 3.16)
project(hiveware LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# --- SOURCES ---
set(SRC
    src/main.c
    src/node/node.c
    src/node/udp.c
    src/node/tcp.c
    src/utils/utils.c
    src/peer_disc/peer_disc.c
    src/client_info/client_info.c
    src/Job/job.c
    src/Job/job_registry.c
)


# --- EXECUTABLE ---
add_executable(hiwa ${SRC})


# --- INCLUDE DIR ---
target_include_directories(hiwa PRIVATE
    src
    src/node
    src/utils
    src/peer_disc
    src/client_info
    src/job
)


# --- WARNING ---
target_compile_options(hiwa PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)


# --- LIBRARIES ---
target_link_libraries(hiwa PRIVATE
    uv_a
    pthread
)

find_library(UUID_LIB uuid)
if(UUID_LIB)
    target_link_libraries(hiwa PRIVATE ${UUID_LIB})
endif()

# libuv
include(FetchContent)

set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)     # only static
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)          # no test

FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG        v1.51.0
)
FetchContent_MakeAvailable(libuv)
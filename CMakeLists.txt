cmake_minimum_required(VERSION 3.16)
project(hiveware LANGUAGES C)

set(CMAKE_C_STANDARD 11) 
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wno-error)

# --- SOURCES ---
set(SRC
    src/main.c
    src/node/node.c
)

# --- EXECUTABLE ---
add_executable(hiwa ${SRC})

# --- INCLUDE DIR ---
target_include_directories(hiwa PRIVATE
    src
    src/node
)

# --- WARNING ---
target_compile_options(hiwa PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# --- LIBRARIES ---
target_link_libraries(hiwa pthread)

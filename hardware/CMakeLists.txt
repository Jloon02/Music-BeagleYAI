# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(final_project 
    VERSION 1.0 
    DESCRIPTION "Final Project" 
    LANGUAGES C
)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")

add_compile_options(-Wall -Wpedantic -Wextra) #-Werror
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(lgpio)
add_subdirectory(lcd)
add_subdirectory(hal)  
add_subdirectory(app)

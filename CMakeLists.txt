cmake_minimum_required(VERSION 3.10)
project(ArchitectureProject)

find_package(Threads REQUIRED)
add_executable(main code.c)
target_link_libraries(main PRIVATE Threads::Threads)

cmake_minimum_required(VERSION 4.1)
project(Graph_Movement_Simulation C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Graph_Movement_Simulation
        graph.c
        graph.h
        main.c
)

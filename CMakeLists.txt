cmake_minimum_required(VERSION 3.25)
project(AutoBuildLayout CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geode REQUIRED)

geode_target(AutoBuildLayout)

target_sources(AutoBuildLayout PRIVATE
    src/main.cpp
)

target_include_directories(AutoBuildLayout PRIVATE
    src
)

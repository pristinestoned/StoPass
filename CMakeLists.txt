cmake_minimum_required(VERSION 3.16)
project(StoPass)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    main.cpp
    utils.cpp
)

add_executable(StoPass WIN32 ${SOURCES})
target_link_libraries(StoPass Qt6::Widgets)

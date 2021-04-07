
cmake_minimum_required(VERSION 3.14)
project(launcher)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(launcher "")
target_sources(launcher
  PRIVATE
  main.cpp
)
find_package(Qt6 COMPONENTS Core)
target_link_libraries(launcher
  PRIVATE
  Qt6::Core
)

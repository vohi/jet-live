
cmake_minimum_required(VERSION 3.14)
project(qtdialog)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(qtdialog "")
target_sources(qtdialog
  PRIVATE
  src/main.cpp
  src/dialog.cpp src/dialog.h
  src/wigglywidget.cpp src/wigglywidget.h
  src/qthotreload.cpp src/qthotreload.h
  )
target_include_directories(qtdialog
  PRIVATE
  src
  )
find_package(Threads REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets)
target_link_libraries(qtdialog
  PRIVATE
  jet-live
  Threads::Threads
  Qt6::Core
  Qt6::Widgets
  Qt6::Gui
  )
set_target_properties(qtdialog
  PROPERTIES
  CXX_STANDARD 11
  )
target_compile_options(qtdialog
  PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:C>:-Wall>
  $<$<COMPILE_LANGUAGE:CXX>:-Wall>
  $<$<COMPILE_LANGUAGE:C>:-Wextra>
  $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
  $<$<COMPILE_LANGUAGE:C>:-Wpedantic>
  $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
  $<$<COMPILE_LANGUAGE:C>:-Wshadow>
  $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
  $<$<COMPILE_LANGUAGE:C>:-Wconversion>
  $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
  )
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(qtdialog
    PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wvexing-parse>
    $<$<COMPILE_LANGUAGE:CXX>:-Wvexing-parse>
    )
endif()

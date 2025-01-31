
cmake_minimum_required(VERSION 3.6)
project(jet-live)

option(JET_LIVE_SHARED "Compile as shared library. ON - shared, OFF - static." OFF)
option(JET_LIVE_BUILD_EXAMPLE "Compile example application." ON)
option(JET_LIVE_BUILD_TESTS "Compile test applications." OFF)

if (NOT JET_LIVE_CONFIGURED)
  include(cmake/jet_live_setup.cmake)
endif()

########## lib_subhook
add_library(lib_subhook STATIC libs/subhook/subhook.c)
target_include_directories(lib_subhook PUBLIC libs/subhook)
# build a static library, so disable declspec(dllimport) and declspec(dllexport)
target_compile_definitions(lib_subhook PUBLIC SUBHOOK_STATIC)

########## lib_whereami
add_library(lib_whereami STATIC "")
target_sources(lib_whereami
  PRIVATE
  libs/whereami/src/whereami.c
  )
target_include_directories(lib_whereami
  PUBLIC
  libs/whereami/src
  )

########## lib_efsw
add_library(lib_efsw STATIC "")
target_sources(lib_efsw
  PRIVATE
  libs/efsw/src/efsw/Debug.cpp
  libs/efsw/src/efsw/DirectorySnapshot.cpp
  libs/efsw/src/efsw/DirectorySnapshotDiff.cpp
  libs/efsw/src/efsw/DirWatcherGeneric.cpp
  libs/efsw/src/efsw/FileInfo.cpp
  libs/efsw/src/efsw/FileSystem.cpp
  libs/efsw/src/efsw/FileWatcher.cpp
  libs/efsw/src/efsw/FileWatcherCWrapper.cpp
  libs/efsw/src/efsw/FileWatcherGeneric.cpp
  libs/efsw/src/efsw/FileWatcherImpl.cpp
  libs/efsw/src/efsw/Log.cpp
  libs/efsw/src/efsw/Mutex.cpp
  libs/efsw/src/efsw/String.cpp
  libs/efsw/src/efsw/System.cpp
  libs/efsw/src/efsw/Thread.cpp
  libs/efsw/src/efsw/Watcher.cpp
  libs/efsw/src/efsw/WatcherGeneric.cpp
)
if (UNIX OR APPLE)
  target_sources(lib_efsw PRIVATE
    libs/efsw/src/efsw/platform/posix/FileSystemImpl.cpp
    libs/efsw/src/efsw/platform/posix/MutexImpl.cpp
    libs/efsw/src/efsw/platform/posix/SystemImpl.cpp
    libs/efsw/src/efsw/platform/posix/ThreadImpl.cpp
  )
endif()

if (UNIX AND NOT APPLE)
  target_sources(lib_efsw PRIVATE
    libs/efsw/src/efsw/FileWatcherInotify.cpp
    libs/efsw/src/efsw/WatcherInotify.cpp
  )
elseif(UNIX AND APPLE)
  target_sources(lib_efsw PRIVATE
    libs/efsw/src/efsw/FileWatcherFSEvents.cpp
    libs/efsw/src/efsw/WatcherFSEvents.cpp
  )
  target_link_libraries(lib_efsw "-framework CoreFoundation" "-framework CoreServices")
elseif(WIN32)
  target_sources(lib_efsw PRIVATE
    libs/efsw/src/efsw/FileWatcherWin32.cpp
    libs/efsw/src/efsw/WatcherWin32.cpp

    libs/efsw/src/efsw/platform/win/FileSystemImpl.cpp
    libs/efsw/src/efsw/platform/win/MutexImpl.cpp
    libs/efsw/src/efsw/platform/win/SystemImpl.cpp
    libs/efsw/src/efsw/platform/win/ThreadImpl.cpp
  )
endif()
target_include_directories(lib_efsw
  PUBLIC
  libs/efsw/include
  libs/efsw/src
  )

########## lib_tiny-process-library
add_library(lib_tiny-process-library STATIC "")
target_sources(lib_tiny-process-library
  PRIVATE
  libs/tiny-process-library/process.cpp
  )
if (UNIX)
target_sources(lib_tiny-process-library
  PRIVATE
  libs/tiny-process-library/process_unix.cpp
  )
elseif (WIN32)
target_sources(lib_tiny-process-library
  PRIVATE
  libs/tiny-process-library/process_win.cpp
  )
endif()
target_include_directories(lib_tiny-process-library
  PUBLIC
  libs/tiny-process-library
  )
set_target_properties(lib_tiny-process-library
  PROPERTIES
  CXX_STANDARD 11
  )

########## lib_teenypath
add_library(lib_teenypath STATIC "")
target_sources(lib_teenypath
  PRIVATE
  libs/teenypath/src/teenypath.cpp
  )
target_include_directories(lib_teenypath
  PUBLIC
  libs/teenypath/include
  )
set_target_properties(lib_teenypath
  PROPERTIES
  CXX_STANDARD 11
  )
if (WIN32)
  target_compile_definitions(lib_teenypath PRIVATE -DUNICODE)
endif()

########## lib_json
add_library(lib_json INTERFACE)
target_include_directories(lib_json
  INTERFACE
  libs/json
  )

########## lib_argh
add_library(lib_argh INTERFACE)
target_include_directories(lib_argh
  INTERFACE
  libs/argh/include
  )

########## lib_xxhash
add_library(lib_xxhash STATIC "")
target_sources(lib_xxhash
  PRIVATE
  libs/xxHash/xxhash.c
  )
target_include_directories(lib_xxhash
  PUBLIC
  libs/xxHash
  )
set_target_properties(lib_xxhash
  PROPERTIES
  CXX_STANDARD 11
  )

if (UNIX AND NOT APPLE)
  ########## lib_elfio
  add_library(lib_elfio INTERFACE)
  target_include_directories(lib_elfio
    INTERFACE
    libs/ELFIO
    )
endif()

########## jet-live
if (JET_LIVE_SHARED)
  add_library(jet-live SHARED "")
else()
  add_library(jet-live STATIC "")
endif()

if (UNIX)
  target_compile_options(jet-live PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_subhook PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_whereami PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_efsw PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_tiny-process-library PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_teenypath PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
  target_compile_options(lib_xxhash PRIVATE $<$<COMPILE_LANGUAGE:C>:-fPIC> $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)
endif()
target_compile_definitions(jet-live PRIVATE JET_LIVE_IMPLEMENTATION)
target_sources(jet-live PRIVATE
  src/jet/live/Utility.cpp
  src/jet/live/DefaultSymbolsFilter.cpp
  src/jet/live/FileWatcher.cpp
  src/jet/live/CompileCommandsCompilationUnitsParser.cpp
  src/jet/live/Compiler.cpp
  src/jet/live/Live.cpp
  src/jet/live/DepfileDependenciesHandler.cpp
  src/jet/live/CodeReloadPipeline.cpp
  src/jet/live/LinkTimeRelocationsStep.cpp
  src/jet/live/FunctionsHookingStep.cpp
  src/jet/live/StaticsCopyStep.cpp
  src/jet/live/SignalReloader.cpp
  src/jet/live/AsyncEventQueue.cpp
  src/jet/live/BuildConfig.cpp
)
if (UNIX AND NOT APPLE)
  target_sources(jet-live PRIVATE
    src/jet/live/_linux/ElfProgramInfoLoader.cpp
    src/jet/live/_linux/Utility.cpp
    )
elseif (UNIX AND APPLE)
  target_sources(jet-live PRIVATE
    src/jet/live/_macos/MachoProgramInfoLoader.cpp
    src/jet/live/_macos/Utility.cpp
    )
elseif (WIN32)
  target_sources(jet-live PRIVATE
    # blink:
    # src/jet/live/_win/coff_reader.cpp
    # src/jet/live/_win/msf_reader.cpp
    # src/jet/live/_win/pdb_reader.cpp
    # src/jet/live/_win/blink.cpp
    # src/jet/live/_win/blink_linker.cpp

    # own:
    src/jet/live/_win/CoffProgramInfoLoader.cpp
    src/jet/live/_win/Utility.cpp
  )

  # target_compile_options(jet-live PUBLIC # public due to static library
    # /GS- # disable security check
    # /Gy  # enable function-level linking
  # )
endif()
target_include_directories(jet-live PUBLIC src/ libs/utils/)
target_link_libraries(jet-live PUBLIC
  lib_subhook
  lib_whereami
  lib_teenypath
  lib_json
  lib_argh
  lib_xxhash
  lib_efsw
  lib_tiny-process-library
)

if (WIN32)
  target_link_libraries(jet-live PUBLIC psapi)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(jet-live PUBLIC dl Threads::Threads)
endif()
if (UNIX AND NOT APPLE)
  target_link_libraries(jet-live PUBLIC lib_elfio)
endif()

set_target_properties(jet-live PROPERTIES CXX_STANDARD 11)

if (UNIX)
target_compile_options(jet-live
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
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(jet-live
    PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wvexing-parse>
    $<$<COMPILE_LANGUAGE:CXX>:-Wvexing-parse>
    )
endif()
set_property(
  SOURCE src/jet/live/BuildConfig.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  JET_LIVE_CMAKE_GENERATOR="${CMAKE_GENERATOR}"
  JET_LIVE_CMAKE_BUILD_DIR="${CMAKE_BINARY_DIR}"
  )

if (MSVC)
  # target_compile_options(jet-live PRIVATE
    # /GS- # Disable Security Check
    # /Gy  # Enable Function-Level Linking
  # )

  # target_compile_definitions(jet-live PRIVATE PSAPI_VERSION=1)
endif()

if (JET_LIVE_BUILD_EXAMPLE)
  add_subdirectory(example)
  add_subdirectory(qt-example)
  add_subdirectory(launcher)
endif()

if (JET_LIVE_BUILD_TESTS)
  add_subdirectory(tests)
endif()


#pragma once

#if !defined JET_LIVE_EXPORT
  #if defined(WIN32) || defined(__WIN32) || defined(__WIN32__)
    #if defined JET_LIVE_IMPLEMENTATION
      #define JET_LIVE_EXPORT __declspec(dllexport)
    #else
      #define JET_LIVE_EXPORT __declspec(dllimport)
    #endif
  #elif defined(__linux__) || defined(__APPLE__)
    #if defined JET_LIVE_IMPLEMENTATION
      #define JET_LIVE_EXPORT __attribute__((visibility("default")))
    #else
      #define JET_LIVE_EXPORT
    #endif
  #endif
#endif

#pragma once

#include <csignal>

#if !defined(JET_LIVE_RELOAD_SIGNAL) && !defined(JET_LIVE_RESTART_SIGNAL)
#  if defined(__linux__) || defined(__APPLE__)
#    define JET_LIVE_RELOAD_SIGNAL SIGUSR1
#    define JET_LIVE_RESTART_SIGNAL SIGUSR2
#  elif defined(__WIN32) || defined(WIN32) || defined(__WIN32__)
// SIGUSR* aren't available on Windows, so use some of the rarely used ones
#    define JET_LIVE_RELOAD_SIGNAL SIGFPE
#    define JET_LIVE_RESTART_SIGNAL SIGILL
#  else
#    error "Unsupported platform"
#  endif
#endif

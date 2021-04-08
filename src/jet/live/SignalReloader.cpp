
#include "SignalReloader.hpp"
#include "jet/live/Live.hpp"

#if __has_include(<csignal>)
#include <csignal>
#endif

namespace
{
    jet::Live* livePtr = nullptr;
}

void signalHandler(int)
{
    if (livePtr) {
        livePtr->tryReload();
    }
}

namespace jet
{
    void onLiveCreated(Live* live, bool reloadOnSignal)
    {
        ::livePtr = live;
#if defined(__LINUX__) || defined(__APPLE__)
        if (reloadOnSignal) {
            signal(SIGUSR1, signalHandler);
        }
#endif
    }

    void onLiveDestroyed() { ::livePtr = nullptr; }
}

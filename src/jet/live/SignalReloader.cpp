
#include "SignalReloader.hpp"
#include <csignal>
#include <iostream>
#include "jet/live/Live.hpp"

#include "signals.hpp"

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
        if (reloadOnSignal) {
            signal(JET_LIVE_RELOAD_SIGNAL, signalHandler);
        }
    }

    void onLiveDestroyed() { ::livePtr = nullptr; }
}

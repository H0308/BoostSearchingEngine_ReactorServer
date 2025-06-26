#ifndef __rs_signal_ign_h__
#define __rs_signal_ign_h__

#include <signal.h>

namespace bs_signal_ign
{
    class SignalIgn
    {
    public:
        SignalIgn()
        {
            signal(SIGPIPE, SIG_IGN);
        }
    };
}

static bs_signal_ign::SignalIgn s;

#endif
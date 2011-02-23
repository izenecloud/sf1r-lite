/**
 * @file process/common/OnSignal.cpp
 * @author Ian Yang
 * @date Created <2009-10-29 13:19:47>
 * @date Updated <2010-08-19 10:16:59>
 */
#include "OnSignal.h"

#include <core/config.h>

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

#include <algorithm>
#include <vector>
#include <iostream>

#include <cstdlib>
// only register signal for system that supports
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

namespace sf1r
{

void setupDefaultSignalHandlers()
{
#ifdef HAVE_SIGNAL_H
    registerExitSignal(SIGINT);
    registerExitSignal(SIGQUIT);
    registerExitSignal(SIGTERM);
    registerExitSignal(SIGHUP);

    // workaground for sdb SegFault
    registerExitSignal(SIGSEGV);
#  if !defined(SF1_IGNORE_SIGNAL) && !defined(DEBUG)
    // do not ignore signals in debug mode
    registerExceptionSignal(SIGBUS);
    registerExceptionSignal(SIGABRT);
# endif
#endif // HAVE_SIGNAL_H
}

namespace   // {anonymous}
{

/**
 * @brief get global OnExit in current compilation unit
 */
std::vector<OnSignalHook>& gOnExit()
{
    static std::vector<OnSignalHook> onExit;
    return onExit;
}

void gForceExit(int)
{
    _exit(1);
}

void gRunHooksOnExitWithSignal(int signal)
{
#ifdef HAVE_SIGNAL_H
    // unregister SIGBUS, SIGABRT, SIGSEGV
    struct ::sigaction action;
    action.sa_handler = gForceExit;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGBUS, &action, 0);
    sigaction(SIGABRT, &action, 0);
    sigaction(SIGSEGV, &action, 0);
#endif

    try
    {
        std::for_each(
            gOnExit().begin(),
            gOnExit().end(),
            boost::bind(boost::apply<void>(), _1, signal)
        );
    }
    catch (...)
    {
        std::cerr << "Uncaught Exception on exit." << std::endl;
        _exit(1);
    }
}
} // namespace {anonymous}

void gRunHooksOnExit()
{
    gRunHooksOnExitWithSignal(0);
}

void addExitHook(const OnSignalHook& hook)
{
    gOnExit().push_back(hook);
}

void registerExitSignal(int signal)
{
    if (signal == 0)
    {
        atexit(gRunHooksOnExit);
        return;
    }

#ifdef HAVE_SIGNAL_H
    struct ::sigaction action;
    action.sa_handler = gRunHooksOnExitWithSignal;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(signal, &action, 0);
#endif
}

void gThrowException(int)
{
    throw std::runtime_error("bad malloc" );
}

void registerExceptionSignal(int signal)
{
#ifdef HAVE_SIGNAL_H
    struct ::sigaction action;
    action.sa_handler = gThrowException;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(signal, &action, 0);
#endif
}

} // namespace sf1r



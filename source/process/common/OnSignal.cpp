/**
 * @file process/common/OnSignal.cpp
 * @author Ian Yang
 * @date Created <2009-10-29 13:19:47>
 * @date Updated <2010-08-19 10:16:59>
 */
#include "OnSignal.h"

#include <glog/logging.h>
#include <core/config.h>

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

#include <algorithm>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <cstdlib>
// only register signal for system that supports
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

static sigset_t maskset;
static pthread_t sig_thread_id;

namespace sf1r
{

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

void gRunHooksOnExitWithSignal(int signal)
{
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

static void* sig_thread(void* arg)
{
    sigset_t *set = (sigset_t *)arg;
    int s, sig;
    for(;;)
    {
        s = sigwait(set, &sig);
        if (s != 0)
        {
            perror("wait signal failed.");
            exit(1);
        }
        switch(sig)
        {
        case SIGINT:
        case SIGHUP:
        case SIGTERM:
        case SIGQUIT:
            LOG(INFO) << "got interrupt signal, begin quit : " << sig << std::endl;
            gRunHooksOnExitWithSignal(sig);
            return 0;
        default:
            LOG(INFO) << "got signal, ignore: " << sig << std::endl;
            break;
        }
    }
    return 0;
}

void setupDefaultSignalHandlers()
{
#ifdef HAVE_SIGNAL_H
    LOG(INFO) << "begin setup signal handler." << std::endl;
    sigemptyset(&maskset);
    sigaddset(&maskset, SIGINT);
    sigaddset(&maskset, SIGHUP);
    sigaddset(&maskset, SIGTERM);
    sigaddset(&maskset, SIGQUIT);
    int ret;
    ret = pthread_sigmask(SIG_BLOCK, &maskset, NULL);
    if (ret != 0)
    {
        perror("failed to block signal!");
        exit(1);
    }
    ret = pthread_create(&sig_thread_id, NULL, &sig_thread, (void*)&maskset);
    
    if (ret != 0)
    {
        perror("failed to create the singal handle thread.");
        exit(1);
    }
#endif // HAVE_SIGNAL_H
}

void waitSignalThread()
{
    pthread_join(sig_thread_id, NULL);
}

void gRunHooksOnExit()
{
    gRunHooksOnExitWithSignal(0);
}

void addExitHook(const OnSignalHook& hook)
{
    gOnExit().push_back(hook);
}

} // namespace sf1r

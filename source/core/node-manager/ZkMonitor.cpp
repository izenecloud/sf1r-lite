#include "ZkMonitor.h"

#include <iostream>

namespace sf1r {

const static long MONOTOR_INTERVAL_SECONDS = 59;

ZkMonitor::ZkMonitor()
: monitorInterval_(MONOTOR_INTERVAL_SECONDS)
{
    thread_ = boost::thread(&ZkMonitor::loopMonitor_, this);
}

ZkMonitor::~ZkMonitor()
{
    stop();
}

void ZkMonitor::addMonitorHandler(monitor_handler_t handler)
{
    monitorHandlers_.push_back(handler);
}

void ZkMonitor::start()
{
    // started in constructor
}

void ZkMonitor::stop()
{
    thread_.interrupt();
    thread_.join();
}

void ZkMonitor::loopMonitor_()
{
    std::cout << "[ZkMonitor] started " <<std::endl;

    while (true)
    {
        try
        {
            boost::this_thread::sleep(boost::posix_time::seconds(monitorInterval_));
            doMonitor_();
        }
        catch (std::exception& e)
        {
            //break;
        }
    }
}

void ZkMonitor::doMonitor_()
{
    std::vector<monitor_handler_t>::iterator it;
    for (it = monitorHandlers_.begin(); it != monitorHandlers_.end(); it++)
    {
        (*it)();
    }
}

}

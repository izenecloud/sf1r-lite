#include "WorkerDetector.h"

#include <boost/algorithm/string/predicate.hpp>

using namespace sf1r;

void WorkerDetector::process(ZooKeeperEvent& zkEvent)
{
    // do something
}

void WorkerDetector::onNodeCreated(const std::string& path)
{
    if (boost::starts_with(path, "/SF1/Worker"))
    {
        // do something
    }
}

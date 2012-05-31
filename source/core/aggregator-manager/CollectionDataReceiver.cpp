#include "CollectionDataReceiver.h"
#include <net/distribute/DataReceiver2.hpp>

namespace sf1r{

void CollectionDataReceiver::init(
        unsigned int port,
        const std::string& colBaseDir)
{
    // init once with double check
    boost::unique_lock<boost::mutex> lock(mutex_init_);
    if (isInited_) {
        return;
    }

    dataReceiver_.reset(
            new izenelib::net::distribute::DataReceiver2(colBaseDir, port)
    );
    isInited_ = true;
}

void CollectionDataReceiver::start()
{
    if (isInited_)
    {
        // start once
        boost::unique_lock<boost::mutex> lock(mutex_start_);
        if (isStarted_) {
            return;
        }

        // receive thread
        dataReceiver_->start();
        isStarted_ = true;
    }
    else
    {
        std::cout<<"Failed to start CollectionDataReceiver : not initialized!"<<std::endl;
    }
}

void CollectionDataReceiver::stop()
{
    // Stop receiver and thread will exit
    dataReceiver_->stop();
}

}

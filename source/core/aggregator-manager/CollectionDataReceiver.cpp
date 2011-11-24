#include "CollectionDataReceiver.h"

using namespace net::distribute;

namespace sf1r{

void CollectionDataReceiver::init(
        unsigned int port,
        const std::string& colBaseDir,
        DataReceiver::buf_size_t bufSize,
        size_t threadNum)
{
    // init once with double check
    boost::unique_lock<boost::mutex> lock(mutex_init_);
    if (isInited_) {
        return;
    }

    dataReceiver_.reset(
            new DataReceiver(port, colBaseDir, bufSize, threadNum)
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
        recvThread_ = boost::thread(&DataReceiver::start, dataReceiver_);
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

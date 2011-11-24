#ifndef COLLECTION_DATA_RECEIVER_H_
#define COLLECTION_DATA_RECEIVER_H_

#include <util/singleton.h>
#include <net/distribute/DataReceiver.h>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

using namespace net::distribute;

namespace sf1r{

class CollectionDataReceiver
{
public:
    CollectionDataReceiver()
    : isInited_(false), isStarted_(false)
    {}

    static CollectionDataReceiver* get()
    {
        return izenelib::util::Singleton<CollectionDataReceiver>::get();
    }

    void init(
            unsigned int port,
            const std::string& colBaseDir,
            DataReceiver::buf_size_t bufSize=DataReceiver::DEFAULT_BUFFER_SIZE,
            size_t threadNum=5);

    void start();

    void stop();

private:
    bool isInited_;
    boost::mutex mutex_init_;
    bool isStarted_;
    boost::mutex mutex_start_;

    boost::shared_ptr<DataReceiver> dataReceiver_;
    boost::thread recvThread_;
};

}

#endif /* COLLECTION_DATA_RECEIVER_H_ */

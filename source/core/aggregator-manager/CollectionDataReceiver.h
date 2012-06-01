#ifndef COLLECTION_DATA_RECEIVER_H_
#define COLLECTION_DATA_RECEIVER_H_

#include <util/singleton.h>

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

namespace izenelib { namespace net { namespace distribute {
class DataReceiver2;
}}}

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
            const std::string& colBaseDir);

    void start();

    void stop();

private:
    bool isInited_;
    boost::mutex mutex_init_;
    bool isStarted_;
    boost::mutex mutex_start_;

    boost::shared_ptr<izenelib::net::distribute::DataReceiver2> dataReceiver_;
};

}

#endif /* COLLECTION_DATA_RECEIVER_H_ */

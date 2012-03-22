/**
 * @file Sf1WorkerController.h
 * @brief a base class to handle request to worker server
 * @author Zhongxia Li, Jun Jiang
 * @date 2012-03-21
 */

#ifndef SF1_WORKER_CONTROLLER_H_
#define SF1_WORKER_CONTROLLER_H_

#include <net/aggregator/WorkerController.h>
#include <common/CollectionManager.h>

#include <string>

namespace sf1r
{
class CollectionHandler; 

class Sf1WorkerController : public net::aggregator::WorkerController
{
public:
    Sf1WorkerController();

    ~Sf1WorkerController();

    virtual bool preprocess(msgpack::rpc::request& req, std::string& error);

protected:
    virtual bool checkWorker(std::string& error) { return true; }

private:
    bool parseCollectionName(msgpack::rpc::request& req, std::string& error);
    bool checkCollectionHandler(std::string& error);

protected:
    std::string collectionName_;
    CollectionHandler* collectionHandler_;

private:
    CollectionManager::MutexType* mutex_;
};

} // namespace sf1r

#endif // SF1_WORKER_CONTROLLER_H_

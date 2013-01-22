#ifndef SF1R_DISTRIBUTE_DRIVER_H
#define SF1R_DISTRIBUTE_DRIVER_H

#include <util/driver/Router.h>
#include <util/driver/Request.h>
#include <util/singleton.h>
#include <util/concurrent_queue.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

namespace sf1r
{

class DistributeDriver
{
public:
    typedef boost::shared_ptr<izenelib::driver::Router> RouterPtr;
    static DistributeDriver* get()
    {
        return izenelib::util::Singleton<DistributeDriver>::get();
    }

    DistributeDriver();
    void init(const RouterPtr& router);
    void stop();
    bool on_new_req_available();
    bool handleReqFromPrimary(const std::string& reqjsondata, const std::string& packed_data);
    bool handleReqFromLog(const std::string& reqjsondata, const std::string& packed_data);

private:
    bool handleRequest(const std::string& reqjsondata, const std::string& packed_data, izenelib::driver::Request::kCallType calltype);

    void run();
    RouterPtr router_;
    boost::thread  async_task_worker_;
    izenelib::util::concurrent_queue<boost::function<void()> > asyncWriteTasks_;
};

}

#endif

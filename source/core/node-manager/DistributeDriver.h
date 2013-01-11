#ifndef SF1R_DISTRIBUTE_DRIVER_H
#define SF1R_DISTRIBUTE_DRIVER_H

#include <util/driver/Router.h>
#include <util/driver/Reader.h>
#include <util/driver/Writer.h>
#include <util/singleton.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

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

    void init(const RouterPtr& router);
    void on_new_req_available();

private:
    RouterPtr router_;
};

}

#endif

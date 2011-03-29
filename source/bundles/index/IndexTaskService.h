#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include <util/osgi/IService.h>

namespace sf1r
{
class IndexTaskService : public ::izenelib::osgi::IService
{
public:
    IndexTaskService();

    ~IndexTaskService();

    void buildCollection(unsigned int numdoc);

    void optimizeIndex();

};

}

#endif


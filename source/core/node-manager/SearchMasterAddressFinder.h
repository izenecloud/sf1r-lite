#ifndef SF1R_SEARCH_MASTER_ADDRESS_FINDER_H
#define SF1R_SEARCH_MASTER_ADDRESS_FINDER_H

#include <string>
#include <common/inttypes.h>

namespace sf1r
{

class SearchMasterAddressFinder
{
public:
    virtual ~SearchMasterAddressFinder() {}

    virtual bool findSearchMasterAddress(std::string& host, uint32_t& port) = 0;
};

} // namespace sf1r

#endif // SF1R_SEARCH_MASTER_ADDRESS_FINDER_H

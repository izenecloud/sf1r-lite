///
/// @file SearchMasterAddressFinderStub.h
/// @brief SearchMasterAddressFinder stub used to test RemoteItemManager
/// @author Jun Jiang
/// @date Created 2012-02-28
///

#ifndef SEARCH_MASTER_ADDRESS_FINDER_STUB_H
#define SEARCH_MASTER_ADDRESS_FINDER_STUB_H

#include <node-manager/SearchMasterAddressFinder.h>

namespace sf1r
{

struct SearchMasterAddressFinderStub : public SearchMasterAddressFinder
{
    std::string host_;
    uint32_t port_;

    SearchMasterAddressFinderStub(const std::string& host, uint32_t port)
        : host_(host)
        , port_(port)
    {}

    virtual bool findSearchMasterAddress(std::string& host, uint32_t& port)
    {
        host = host_;
        port = port_;

        return true;
    }
};

} // namespace sf1r

#endif // SEARCH_MASTER_ADDRESS_FINDER_STUB_H

#ifndef SF1R_PRODUCT_BUNDLE_PRODUCTSCDRECEIVER_H
#define SF1R_PRODUCT_BUNDLE_PRODUCTSCDRECEIVER_H

#include <sstream>
#include <common/type_defs.h>

#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <product-manager/pm_def.h>
#include <product-manager/pm_types.h>

#include <node-manager/synchro/SynchroFactory.h>

namespace sf1r
{
class IndexTaskService;
class ProductScdReceiver
{
public:

    ProductScdReceiver(const std::string& syncID, const std::string& collectionName, const std::string& callback);

    void Set(IndexTaskService* index_service)
    {
        index_service_ = index_service;
    }

    bool onReceived(const std::string& scd_source_dir);

    bool Run(const std::string& scd_source_dir);

private:

    bool CopyFileListToDir_(const std::vector<boost::filesystem::path>& file_list, const boost::filesystem::path& to_dir);

    bool NextScdFileName_(std::string& filename) const;
    bool pushIndexRequest(const std::string& scd_source_dir);

private:
    IndexTaskService* index_service_;

    std::string syncID_;
    std::string collectionName_;
    std::string callback_type_;
    SynchroConsumerPtr syncConsumer_;
};

}

#endif

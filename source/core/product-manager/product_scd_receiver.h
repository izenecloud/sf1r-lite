#ifndef SF1R_PRODUCTMANAGER_PRODUCTSCDRECEIVER_H
#define SF1R_PRODUCTMANAGER_PRODUCTSCDRECEIVER_H

#include <sstream>
#include <common/type_defs.h>

#include <boost/operators.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "pm_def.h"
#include "pm_types.h"
#include "distributed_process_synchronizer.h"

namespace sf1r
{
class IndexTaskService;
class ProductScdReceiver 
{
public:
    
    ProductScdReceiver();
    
    void Set(IndexTaskService* index_service)
    {
        index_service_ = index_service;
    }
    
    void Run(const std::string& scd_source_dir);
    
private:
    
    bool CopyFileListToDir_(const std::vector<boost::filesystem::path>& file_list, const boost::filesystem::path& to_dir);
    
    bool NextScdFileName_(std::string& filename) const;
        
    
private:
    IndexTaskService* index_service_;
    DistributedProcessSynchronizer dsSyn;
};

}

#endif


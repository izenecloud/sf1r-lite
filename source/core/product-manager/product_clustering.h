#ifndef SF1R_PRODUCTMANAGER_PRODUCTCLUSTERING_H
#define SF1R_PRODUCTMANAGER_PRODUCTCLUSTERING_H

#include <common/type_defs.h>

#include <idmlib/duplicate-detection/dup_detector.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{
    
class ProductClusteringPostItem
{
public:
    std::string docid;
    izenelib::util::UString category;
    ProductPrice price;
    bool valid;
    
    bool Sim(const ProductClusteringPostItem& other) const
    {
        if(!valid || !other.valid) return false;
        if(category!=other.category) return false;
        ProductPriceType mid1;
        ProductPriceType mid2;
        if(!price.GetMid(mid1)) return false;
        if(!other.price.GetMid(mid2)) return false;
        ProductPriceType max = std::max(mid1, mid2);
        ProductPriceType min = std::min(mid1, mid2);
        if(min<=0.0) return false;
        double ratio = max/min;
        if(ratio>1.8) return false;
        return true;
    }
};

class ProductClustering
{
public:
    ProductClustering(const std::string& work_dir, const PMConfig& config);

    ~ProductClustering();

    bool Open();
    
    void Insert(const PMDocumentType& doc);

    bool Run();
    
    idmlib::dd::GroupTable* GetGroupTable() const
    {
        return group_table_;
    }

    
    inline const std::string& GetLastError() const
    {
        return error_;
    }

    inline const PMConfig& GetConfig() const
    {
        return config_;
    }

private:
    
private:
    std::string work_dir_;
    PMConfig config_;
    std::string error_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::dd::DupDetector* dd_;
    idmlib::dd::GroupTable* group_table_;
};

}

#endif

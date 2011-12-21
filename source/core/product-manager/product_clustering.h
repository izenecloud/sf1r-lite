#ifndef SF1R_PRODUCTMANAGER_PRODUCTCLUSTERING_H
#define SF1R_PRODUCTMANAGER_PRODUCTCLUSTERING_H

#include <common/type_defs.h>

#include <idmlib/duplicate-detection/dup_detector.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include "product_term_analyzer.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{

class ProductClusteringAttach
{
public:
//     std::string docid;
    izenelib::util::UString category;
    izenelib::util::UString city;
    ProductPrice price;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & category & city & price;
    }

    bool dd(const ProductClusteringAttach& other) const
    {
        if(category!=other.category) return false;
        if(city!=other.city) return false;
        ProductPriceType mid1;
        ProductPriceType mid2;
        if(!price.GetMid(mid1)) return false;
        if(!other.price.GetMid(mid2)) return false;
        ProductPriceType max = std::max(mid1, mid2);
        ProductPriceType min = std::min(mid1, mid2);
        if(min<=0.0) return false;
        double ratio = max/min;
        if(ratio>1.5) return false;
        return true;
    }
};

class ProductClustering
{
public:
    typedef idmlib::dd::DupDetector<std::string, uint32_t, ProductClusteringAttach> DDType;
    typedef DDType::GroupTableType GroupTableType;
    ProductClustering(const std::string& work_dir, const PMConfig& config);

    ~ProductClustering();

    bool Open();

    void Insert(const PMDocumentType& doc);

    bool Run();

    GroupTableType* GetGroupTable() const
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
    std::string work_dir_;
    PMConfig config_;
    std::string error_;
    ProductTermAnalyzer analyzer_;
    DDType* dd_;
    GroupTableType* group_table_;
};

}

#endif

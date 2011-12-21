#ifndef SF1R_PRODUCTMANAGER_PRODUCTTERMANALYZER_H
#define SF1R_PRODUCTMANAGER_PRODUCTTERMANALYZER_H

#include <common/type_defs.h>

#include <idmlib/util/idm_analyzer.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"


namespace sf1r
{

class ProductTermAnalyzer
{
    
    enum TERM_STATUS {NORMAL, IN_BRACKET, IN_MODEL};
    
public:
    
    ProductTermAnalyzer();

    ~ProductTermAnalyzer();

    void Analyze(const izenelib::util::UString& title, std::vector<std::string>& terms, std::vector<double>& weights);

    
private:
    
    double GetWeight_(const izenelib::util::UString& all, const izenelib::util::UString& term, char tag);
    
private:
    idmlib::util::IDMAnalyzer* analyzer_;
};

}

#endif

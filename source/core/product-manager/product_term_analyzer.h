#ifndef SF1R_PRODUCTMANAGER_PRODUCTTERMANALYZER_H
#define SF1R_PRODUCTMANAGER_PRODUCTTERMANALYZER_H

#include <boost/unordered_set.hpp>
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
    ProductTermAnalyzer(const std::string& cma_path="");

    ~ProductTermAnalyzer();

    void Analyze(const izenelib::util::UString& title, std::vector<std::string>& terms, std::vector<double>& weights);

private:
    double GetWeight_(uint32_t title_length, const izenelib::util::UString& term, char tag);

private:
    idmlib::util::IDMAnalyzer* analyzer_;
    boost::unordered_set<std::string> stop_set_;
};

}

#endif

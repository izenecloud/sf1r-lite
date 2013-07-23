#ifndef SF1R_B5MMANAGER_B5MPPROCESSOR2_H_
#define SF1R_B5MMANAGER_B5MPPROCESSOR2_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "product_db.h"
#include "offer_db.h"
#include "brand_db.h"
//#include "history_db_helper.h"
#include <common/ScdMerger.h>
#include <common/PairwiseScdMerger.h>
#include <am/sequence_file/ssfr.h>
#include <util/DynamicBloomFilter.h>

namespace sf1r {

    ///B5mpProcessor is responsibility to generate b5mp scds and also b5mo_mirror scd
    class B5mpProcessor2{
    public:
        B5mpProcessor2(const std::string& m, const std::string& last_m)
        :m_(m), last_m_(last_m)
        {
        }

        bool Generate(bool spu_only=false)
        {
            LOG(INFO)<<"b5mp merger begin"<<std::endl;
            B5moSorter sorter(m_);
            bool succ = sorter.StageTwo(spu_only, last_m_);
            LOG(INFO)<<"b5mp merger finish"<<std::endl;
            return succ;
        }

    private:

    private:
        std::string m_;
        std::string last_m_;
    };

}

#endif



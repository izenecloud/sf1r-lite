#ifndef SF1R_B5MMANAGER_B5MPUUEPROCESSOR_H_
#define SF1R_B5MMANAGER_B5MPUUEPROCESSOR_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include "b5m_types.h"
#include "product_db.h"

namespace sf1r {
    class B5MPUueProcessor{
        typedef boost::unordered_map<std::string, UueFromTo > O2PMap;
        //struct ProductValue {
            //uint32_t itemcount;
        //};
        //typedef boost::unorder_map<std::string, ProductValue> ProductMap;
    public:
        B5MPUueProcessor(const std::string& b5mo_scd, const std::string& b5mp_scd, ProductDb* product_db, const std::string& work_path, bool reindex = false, bool fast_mode = true);

        void Process(const UueItem& item);

        void Finish();

    private:


    private:
        std::string b5mo_scd_;
        std::string b5mp_scd_;
        ProductDb* product_db_;
        std::string work_path_;
        bool reindex_;
        bool fast_mode_;
        O2PMap o2p_map_;
    };

}

#endif


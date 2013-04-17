#ifndef SF1R_B5MMANAGER_TUANPROCESSOR_H
#define SF1R_B5MMANAGER_TUANPROCESSOR_H 
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <product-manager/product_price.h>
#include "b5m_helper.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <common/PairwiseScdMerger.h>
#include "pid_dictionary.h"

namespace sf1r {

    class TuanProcessor{
        typedef izenelib::util::UString UString;

        class TuanProcessorAttach
        {
        public:
            uint32_t sid; //sid should be same
            ProductPrice price;

            template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar & sid & price;
            }

            bool dd(const TuanProcessorAttach& other) const
            {
                if(sid!=other.sid) return false;
                return true;
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
        typedef std::string DocIdType;

        struct ValueType
        {
            std::string soid;
            UString title;

            bool operator<(const ValueType& other) const
            {
                return title.length()<other.title.length();
            }
            
        };


        typedef idmlib::dd::DupDetector<DocIdType, uint32_t, TuanProcessorAttach> DDType;
        typedef DDType::GroupTableType GroupTableType;
        typedef PairwiseScdMerger::ValueType SValueType;
        typedef boost::unordered_map<uint128_t, SValueType> CacheType;

    public:
        TuanProcessor();
        bool Generate(const std::string& scd_path, const std::string& mdb_instance);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:

        void POutputAll_();
        void B5moOutput_(SValueType& value, int status);
        uint128_t GetPid_(const Document& doc);
        uint128_t GetOid_(const Document& doc);
        void ProductMerge_(SValueType& value, const SValueType& another_value);

    private:
        std::string cma_path_;
        CacheType cache_;
        boost::shared_ptr<ScdWriter> pwriter_;
    };

}

#endif


#ifndef SF1R_B5MMANAGER_B5MPPROCESSOR_H_
#define SF1R_B5MMANAGER_B5MPPROCESSOR_H_

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

    class LogServerConnectionConfig;
    ///B5mpProcessor is responsibility to generate b5mp scds and also b5mo_mirror scd
    class B5mpProcessor{
        //typedef izenelib::am::ssf::Writer<> PoMapWriter;
        //typedef std::set<std::string> ItemsT;
        //typedef boost::unordered_map<std::string, ItemsT >  ProductOfferT;
        typedef PairwiseScdMerger::ValueType ValueType;
        typedef std::pair<ValueType, ValueType> PValueType;
        typedef boost::unordered_map<uint128_t, PValueType> CacheType;
        typedef CacheType::iterator CacheIterator;
        typedef izenelib::util::BloomFilter<uint128_t, uint128_t> FilterType;
        //struct PValueType
        //{
        //};
    public:
        B5mpProcessor(const std::string& mdb_instance,
            const std::string& last_mdb_instance);

        bool Generate();

    private:

        void FilterTask_(ValueType& value);

        bool B5moValid_(const Document& doc);

        void OutputAll_();

        void ProductMerge_(ValueType& value, const ValueType& another_value);

        void GetOutputP_(ValueType& value);
        void ProductOutput_(PValueType& pvalue);

        void Preprocess_(ValueType& value);

        void B5moOutput_(ValueType& value, int status);
        void B5moPost_(ValueType& value, int status);                           


        static uint128_t GetPid_(const Document& doc);
        static uint128_t GetOid_(const Document& doc);

    private:
        std::string mdb_instance_;
        std::string last_mdb_instance_;
        CacheType cache_;
        boost::shared_ptr<ScdTypeWriter> writer_;
        boost::shared_ptr<FilterType> filter_; //pfilter
        boost::shared_ptr<FilterType> ofilter_; //ofilter
        std::vector<std::string> random_properties_;
        std::size_t odoc_count_;

        //BrandDb* bdb_;
    };

}

#endif


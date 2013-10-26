#ifndef SF1R_B5MMANAGER_TICKETPROCESSOR_H
#define SF1R_B5MMANAGER_TICKETPROCESSOR_H 
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <common/PairwiseScdMerger.h>
NS_SF1R_B5M_BEGIN

class TicketProcessor{
    typedef izenelib::util::UString UString;

    class TicketProcessorAttach
    {
    public:
        uint32_t sid; //sid should be same
        std::vector<std::string> time_array;//sorted, should be at least one same

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & sid & time_array;
        }

        bool dd(const TicketProcessorAttach& other) const
        {
            if(sid!=other.sid) return false;
            bool found = false;
            uint32_t i=0,j=0;
            while(i<time_array.size()&&j<other.time_array.size())
            {
                if(time_array[i]==other.time_array[j])
                {
                    found = true;
                    break;
                }
                else if(time_array[i]<other.time_array[j])
                {
                    ++i;
                }
                else
                {
                    ++j;
                }
            }
            if(!found) return false;
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


    typedef idmlib::dd::DupDetector<DocIdType, uint32_t, TicketProcessorAttach> DDType;
    typedef DDType::GroupTableType GroupTableType;
    typedef PairwiseScdMerger::ValueType SValueType;
    typedef boost::unordered_map<uint128_t, SValueType> CacheType;

public:
    TicketProcessor();
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

NS_SF1R_B5M_END

#endif


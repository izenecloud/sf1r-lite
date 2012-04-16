#ifndef SF1R_B5MMANAGER_SIMILARITYMATCHER_H
#define SF1R_B5MMANAGER_SIMILARITYMATCHER_H 
#include <boost/unordered_map.hpp>
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <product-manager/product_price.h>

namespace sf1r {

    class SimilarityMatcher{
        typedef izenelib::util::UString UString;

        class SimilarityMatcherAttach
        {
        public:
            uint32_t id; //id should be different
            izenelib::util::UString category;
            ProductPrice price;

            template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar & id & category & price;
            }

            bool dd(const SimilarityMatcherAttach& other) const
            {
                if(category!=other.category) return false;
                if(id==other.id) return false;
                double mid1;
                double mid2;
                if(!price.GetMid(mid1)) return false;
                if(!other.price.GetMid(mid2)) return false;
                double max = std::max(mid1, mid2);
                double min = std::min(mid1, mid2);
                if(min<=0.0) return false;
                double ratio = max/min;
                if(ratio>2.2) return false;
                return true;
            }
        };
        typedef std::string DocIdType;
        typedef idmlib::dd::DupDetector<DocIdType, uint32_t, SimilarityMatcherAttach> DDType;
        typedef DDType::GroupTableType GroupTableType;

        struct ValueType
        {
            std::string soid;
            UString title;

            bool operator<(const ValueType& other) const
            {
                return title.length()<other.title.length();
            }
            
        };
    public:
        SimilarityMatcher();
        bool Index(const std::string& scd_path, const std::string& knowledge_dir);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

    private:
        std::string cma_path_;
    };

}

#endif


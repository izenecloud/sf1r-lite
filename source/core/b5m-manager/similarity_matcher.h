#ifndef SF1R_B5MMANAGER_SIMILARITYMATCHER_H
#define SF1R_B5MMANAGER_SIMILARITYMATCHER_H 
#include <boost/unordered_map.hpp>
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <product-manager/product_price.h>
#include "pid_dictionary.h"

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

        struct ValueType
        {
            std::string soid;
            UString title;

            bool operator<(const ValueType& other) const
            {
                return title.length()<other.title.length();
            }
            
        };

        template <typename DocIdType, typename GT>
        class SimilarityMatcherGt
        {
        public:
            typedef boost::unordered_map<DocIdType, std::pair<bool, DocIdType> > ResultType;
            SimilarityMatcherGt(PidDictionary* dic): dic_(dic)
            {
            }

            //inline bool IsSymmetric() const
            //{
                //return false;
            //}

            bool AddDoc(const DocIdType& docid1, const DocIdType& docid2, char flag)
            {
                bool result = false;
                //LOG(INFO)<<"flag : "<<(int)flag<<std::endl;
                bool new1 = (flag & 0x01)>0;
                flag >>= 1;
                bool new2 = (flag & 0x01)>0;
                //LOG(INFO)<<"AddDoc "<<docid1<<","<<docid2<<","<<(int)new1<<","<<(int)new2<<std::endl;
                if(new1&&new2)
                {
                    DocIdType small = std::min(docid1, docid2);
                    DocIdType big = std::max(docid1, docid2);
                    if(dic_->Exist(small))
                    {
                        FindMatch_(big, small, true);
                        result = true;
                    }
                    else if(dic_->Exist(big))
                    {
                        FindMatch_(small, big, true);
                        result = true;
                    }
                }
                else if(new1)
                {
                    if(dic_->Exist(docid2))
                    {
                        FindMatch_(docid1, docid2, false);
                        result = true;
                    }
                }
                else if(new2)
                {
                    if(dic_->Exist(docid1))
                    {
                        FindMatch_(docid2, docid1, false);
                        result = true;
                    }
                }
                return result;
            }

            bool RemoveDoc(const DocIdType& docid)
            {
                //do nothing.
                return true;
            }

            bool Flush()
            {
                return true;
            }

            void PrintStat()
            {
                std::cout<<"Find Match count: "<<result.size()<<std::endl;
            }

        public:
            ResultType result;

        private:
            void FindMatch_(const DocIdType& oid, const DocIdType& pid, bool weak)
            {
                std::pair<bool, DocIdType> value(weak, pid);
                typename ResultType::iterator it = result.find(oid);
                if(it==result.end())
                {
                    result.insert(std::make_pair(oid,value));
                    dic_->Erase(oid);
                }
                else
                {
                    if(value<it->second)
                    {
                        dic_->Erase(it->second.second);
                        it->second = value;
                    }
                }
            }

        private:
            PidDictionary* dic_;

        };

        typedef idmlib::dd::DupDetector<DocIdType, uint32_t, SimilarityMatcherAttach, SimilarityMatcherGt> DDType;
        typedef DDType::GroupTableType GroupTableType;

    public:
        SimilarityMatcher();
        bool Index(const std::string& scd_path, const std::string& knowledge_dir);

        void SetCmaPath(const std::string& path)
        { cma_path_ = path; }

        void SetPidDictionary(const std::string& dic_path) 
        {
            dic_path_ = dic_path;
            LOG(INFO)<<"pid dictionary set, path: "<<dic_path<<std::endl;
        }


    private:
        std::string cma_path_;
        std::string dic_path_;
    };

}

#endif


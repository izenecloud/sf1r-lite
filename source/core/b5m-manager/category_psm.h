#ifndef SF1R_B5MMANAGER_CATEGORYPSM_H_
#define SF1R_B5MMANAGER_CATEGORYPSM_H_

#include "psm_helper.h"
#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "bmn_clustering.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <idmlib/duplicate-detection/psm.h>
#include <knlp/cluster_detector.h>
#include <knlp/attr_normalize.h>

NS_SF1R_B5M_BEGIN

class CategoryPsm
{

    typedef std::vector<std::pair<std::string, double> > DocVector;
    typedef idmlib::dd::SimHash SimHash;
    typedef idmlib::dd::CharikarAlgo CharikarAlgo;
    struct BufferItem
    {
        std::string title;
        DocVector doc_vector;
        SimHash fp;
        double price;
        std::vector<std::string> keywords;
        std::vector<std::string> brands;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & title & doc_vector & fp & price & keywords & brands;
        }
        bool operator<(const BufferItem& b) const
        {
            return price<b.price;
        }
    };
    typedef std::vector<BufferItem> BufferValue;
    typedef std::pair<std::string, std::string> BufferKey;
    typedef boost::unordered_map<BufferKey, BufferValue> Buffer;

    struct Group
    {
        std::vector<BufferItem> items;
        uint32_t cindex;
        //SimHash fp;
        double price;
        std::vector<std::string> keywords;
        std::vector<std::string> brands;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & items & cindex & price & keywords & brands;
        }

        bool operator<(const Group& g) const
        {
            return price<g.price;
        }
    };
    typedef boost::unordered_map<BufferKey, std::vector<Group> > ResultMap;
    typedef boost::unordered_map<std::string, double> WeightMap;
public:
    enum FLAG {NO, PENDING, RESULT};
    //typedef idmlib::dd::DupDetector<uint32_t, uint32_t, Attach> DDType;
    //typedef DDType::GroupTableType GroupTableType;
    //typedef idmlib::dd::PSM<64, 3, 24, std::string, std::string, Attach> PsmType;
    CategoryPsm(const std::string& knowledge)
      : gp_(NULL)
      , attr_(NULL)
      , attrn_(NULL)
      , bmn_(NULL)
    {
        LOG(INFO)<<"loading garbage"<<std::endl;
        gp_ = new ilplib::knlp::GarbagePattern(knowledge+"/garbage.pat");
        LOG(INFO)<<"loading detector"<<std::endl;
        attr_ = new ilplib::knlp::ClusterDetector(knowledge+"/product_name.dict", knowledge+"/att.syn", gp_);
        LOG(INFO)<<"loading normalize"<<std::endl;
        attrn_ = new ilplib::knlp::AttributeNormalize(knowledge+"/att.syn");
        LOG(INFO)<<"category psm init finish"<<std::endl;
    }
    ~CategoryPsm()
    {
        if(gp_!=NULL) delete gp_;
        if(attr_!=NULL) delete attr_;
        if(attrn_!=NULL) delete attrn_;
        if(bmn_!=NULL) delete bmn_;
    }

    ilplib::knlp::AttributeNormalize* GetAttributeNormalize() const
    {
        return attrn_;
    }
    bool Open(const std::string& path)
    {
        path_ = path;
        if(boost::filesystem::exists(path))
        {
            std::string p = path+"/bmn";
            bmn_ = new BmnClustering(p);
        }
        return true;
    }

    FLAG Search(const Document& doc, const std::vector<std::string>& brands, const std::vector<std::string>& ckeywords, Product& product)
    {
        bool bmn_valid = false;
        if(bmn_!=NULL&&bmn_->IsValid(doc)) 
        {
            bmn_valid = true;
        }
        if(!bmn_valid)
        {
            std::string scategory;
            std::string stitle;
            std::string sattr;
            doc.getString("Category", scategory);
            doc.getString("Title", stitle);
            doc.getString("Attribute", sattr);
            ProductPrice price;
            UString uprice;
            doc.getString("Price", uprice);
            price.Parse(uprice);
            double dp = 0.0;
            price.GetMid(dp);
            if(dp>0.0)
            {
                std::string ap;
                try {
                    ap = attr_->cluster_detect(stitle, scategory, sattr, (float)dp);
                }
                catch(std::exception& ex)
                {
                    ap = "";
                }
                if(!ap.empty())
                {
                    //std::cerr<<"attr result "<<stitle<<" : "<<ap<<std::endl;
                    product.spid = B5MHelper::GetPidByUrl(ap);
                    product.type = Product::ATTRIB;
                    return RESULT;
                }
            }
            return NO;
        }
        else {
            std::string oid;
            std::string pid;
            doc.getString("DOCID", oid);
            if(bmn_->Get(oid, pid))
            {
                product.spid = pid;
                product.type = Product::FASHION;
                return RESULT;
            }
            else
            {
                bmn_->Append(doc, brands, ckeywords);
                product.type = Product::PENDING;
                return PENDING;
            }
        }

    }
    void BmnFinish(const boost::function<void (ScdDocument&)>& func, int thread_num)
    {
        if(bmn_!=NULL)
        {
            bmn_->Finish(func, thread_num);
        }
    }

    bool Flush(const std::string& path)
    {
        return true;
    }

private:

private:
    std::string path_;
    boost::mutex mutex_;
    ilplib::knlp::GarbagePattern* gp_;
    ilplib::knlp::ClusterDetector* attr_;
    ilplib::knlp::AttributeNormalize* attrn_;
    BmnClustering* bmn_;
};

NS_SF1R_B5M_END

#endif



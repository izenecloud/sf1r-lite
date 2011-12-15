#include "product_clustering.h"
#include <document-manager/Document.h>
#include <la-manager/LAPool.h>
#include <ir/index_manager/index/IndexerDocument.h>

// #define PM_CLUST_TEXT_DEBUG

using namespace sf1r;
namespace bfs = boost::filesystem;
ProductClustering::ProductClustering(const std::string& work_dir, const PMConfig& config)
: work_dir_(work_dir), config_(config)
, analyzer_(NULL), dd_(NULL), group_table_(NULL)
{
    std::string kma_path;
    LAPool::getInstance()->get_kma_path(kma_path );
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path );
    std::string jma_path;
    LAPool::getInstance()->get_jma_path(jma_path );
//     idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig(kma_path,cma_path,jma_path);
    //use cma only
    idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path,"");
    analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
}

bool ProductClustering::Open()
{
    try
    {
        bfs::remove_all(work_dir_);
        bfs::create_directories(work_dir_);
        std::string dd_container = work_dir_ +"/dd_container";
        std::string group_table_file = work_dir_ +"/group_table";
        group_table_ = new GroupTableType(group_table_file);
        group_table_->Load();
        dd_ = new DDType(dd_container, group_table_);
        if(config_.algo_fixk>0)
        {
            LOG(INFO)<<"Set algorithm fixk="<<config_.algo_fixk<<std::endl;
            dd_->SetFixK(config_.algo_fixk);
        }
//         dd_->SetMaxProcessTable(40);
        if(!dd_->Open())
        {
            std::cout<<"DD open failed"<<std::endl;
            return false;
        }
    }
    catch(std::exception& ex)
    {
        std::cout<<ex.what()<<std::endl;
        return false;
    }
    return true;
}

ProductClustering::~ProductClustering()
{
    delete analyzer_;
    if(group_table_!=NULL)
    {
        delete group_table_;
    }
    if(dd_!=NULL)
    {
        delete dd_;
    }
}

void ProductClustering::Insert(const PMDocumentType& doc)
{
    izenelib::util::UString udocid;
    if(!doc.getProperty(config_.docid_property_name, udocid))
    {
        error_ = "DOCID is empty.";
        return;
    }
    izenelib::util::UString title;
    if(!doc.getProperty(config_.title_property_name, title))
    {
        error_ = "Title is empty.";
        return;
    }
    izenelib::util::UString category;
    if(!doc.getProperty(config_.category_property_name, category))
    {
        error_ = "Category is empty.";
        return;
    }
    
    if(title.length()==0 || category.length()==0 )
    {
        error_ = "Title or Category length equals zero.";
        return;
    }
    izenelib::util::UString uprice;
    if(!doc.getProperty(config_.price_property_name, uprice))
    {
        error_ = "Price is empty.";
        return;
    }
    ProductPrice price;
    price.Parse(uprice);
    if(!price.Valid() || price.value.first<=0.0 )
    {
        error_ = "Price is invalid.";
        return;
    }
    
    izenelib::util::UString city;
    doc.getProperty(config_.city_property_name, city);
    ProductClusteringAttach attach;
//     udocid.convertString(attach.docid, izenelib::util::UString::UTF_8);
    attach.category = category;
    attach.city = city;
    attach.price = price;
    std::vector<idmlib::util::IDMTerm> term_list;
    analyzer_->GetTermList(title, term_list);
    std::vector<std::string> v;
    std::vector<double> weights;
    v.reserve(term_list.size());
    weights.reserve(term_list.size());
    
#ifdef PM_CLUST_TEXT_DEBUG
    std::string stitle;
    title.convertString(stitle, izenelib::util::UString::UTF_8);
    std::cout<<"[Title] "<<stitle<<std::endl<<"[Tokens] ";
#endif
    for (uint32_t i=0;i<term_list.size();i++)
    {
        const std::string& str = term_list[i].TextString();
        char tag = term_list[i].tag;
        double weight = GetWeight_(title, term_list[i].text, tag);
        if( weight<=0.0 ) continue;
#ifdef PM_CLUST_TEXT_DEBUG
        std::cout<<"["<<str<<","<<tag<<","<<weight<<"],";
#endif
        v.push_back(str);
        //TODO the weights
        weights.push_back(weight);
    }
#ifdef PM_CLUST_TEXT_DEBUG
    std::cout<<std::endl;
#endif
    if( v.empty() )
    {
        error_ = "Title analyzer result is empty.";
        return;
    }
    std::string docid;
    udocid.convertString(docid, izenelib::util::UString::UTF_8);
    dd_->InsertDoc(docid, v, weights, attach);
    
}

bool ProductClustering::Run()
{
    bool run_dd = dd_->RunDdAnalysis();
    return run_dd;
}

double ProductClustering::GetWeight_(const izenelib::util::UString& all, const izenelib::util::UString& term, char tag)
{
    double weight = 1.0;
    if(tag=='F')
    {
        if(term.length()<2) 
        {
            weight = 0.0;
        }
        else
        {
            weight = 1.5;
        }
    }
    else if(tag=='M')
    {
        weight = 0.6;
    }
    return weight;
}



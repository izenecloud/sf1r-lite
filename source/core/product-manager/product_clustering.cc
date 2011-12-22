#include "product_clustering.h"
#include <document-manager/Document.h>

#include <ir/index_manager/index/IndexerDocument.h>


using namespace sf1r;
namespace bfs = boost::filesystem;
ProductClustering::ProductClustering(const std::string& work_dir, const PMConfig& config)
: work_dir_(work_dir), config_(config)
, dd_(NULL), group_table_(NULL)
{
    
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
    
    std::vector<std::string> terms;
    std::vector<double> weights;
    analyzer_.Analyze(title, terms, weights);
    
    if( terms.empty() )
    {
        error_ = "Title analyzer result is empty.";
        return;
    }
    std::string docid;
    udocid.convertString(docid, izenelib::util::UString::UTF_8);
    dd_->InsertDoc(docid, terms, weights, attach);
}

bool ProductClustering::Run()
{
    bool run_dd = dd_->RunDdAnalysis();
    return run_dd;
}


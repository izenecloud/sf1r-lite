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
    idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig(kma_path,cma_path,jma_path);
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
        group_table_ = new idmlib::dd::GroupTable(group_table_file);
        group_table_->Load();
        dd_ = new idmlib::dd::DupDetector(dd_container, group_table_);
        dd_->SetFixK(3);
        dd_->SetMaxProcessTable(40);
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
    std::vector<izenelib::util::UString> termStrList;
    analyzer_->GetFilteredStringList( title, termStrList );
    std::vector<std::vector<std::string> > v_list(2);
    v_list[0].resize(termStrList.size() );
    v_list[1].resize(1);
#ifdef PM_CLUST_TEXT_DEBUG
    std::string stitle;
    title.convertString(stitle, izenelib::util::UString::UTF_8);
    std::cout<<"[Title] "<<stitle<<std::endl<<"[Tokens] ";
#endif
    for (uint32_t u=0;u<termStrList.size();u++)
    {
        std::string display;
        termStrList[u].convertString(display, izenelib::util::UString::UTF_8);
#ifdef PM_CLUST_TEXT_DEBUG
        std::cout<<display<<",";
#endif
        v_list[0][u] = display;
    }
#ifdef PM_CLUST_TEXT_DEBUG
    std::cout<<std::endl;
#endif
    category.convertString(v_list[1][0], izenelib::util::UString::UTF_8);
    std::string docid;
    udocid.convertString(docid, izenelib::util::UString::UTF_8);
    dd_->InsertDoc(docid, v_list[0]);
//     dd_->InsertDoc(docid, v_list);
    
}

bool ProductClustering::Run()
{
    bool run_dd = dd_->RunDdAnalysis();
    return run_dd;
}


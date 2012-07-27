#include "product_clustering.h"
#include "pm_util.h"

#include <document-manager/Document.h>

#include <ir/index_manager/index/IndexerDocument.h>


using namespace sf1r;
namespace bfs = boost::filesystem;

ProductClustering::ProductClustering(const std::string& work_dir, const PMConfig& config, PMUtil* util)
    : work_dir_(work_dir), config_(config), util_(util)
    , dd_(NULL), group_table_(NULL)
{

}

bool ProductClustering::Open()
{
    try
    {
        bfs::remove_all(work_dir_);
        bfs::create_directories(work_dir_);
        std::string dd_container = work_dir_ + "/dd_container";
        std::string group_table_file = work_dir_ + "/group_table";
        group_table_ = new GroupTableType(group_table_file);
        group_table_->Load();
        dd_ = new DDType(dd_container, group_table_);
        if (config_.algo_fixk > 0)
        {
            LOG(INFO) << "Set algorithm fixk=" << config_.algo_fixk;
            dd_->SetFixK(config_.algo_fixk);
        }
//         dd_->SetMaxProcessTable(40);
        if (!dd_->Open())
        {
            LOG(ERROR) << "DD open failed";
            return false;
        }
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return false;
    }
    return true;
}

ProductClustering::~ProductClustering()
{
    if (group_table_)
    {
        delete group_table_;
    }
    if (dd_)
    {
        delete dd_;
    }
}

void ProductClustering::Insert(const PMDocumentType& doc)
{
    izenelib::util::UString udocid;
    if (!doc.getProperty(config_.docid_property_name, udocid))
    {
        error_ = "DOCID is empty.";
        return;
    }
    izenelib::util::UString title;
    if (!doc.getProperty(config_.title_property_name, title))
    {
        error_ = "Title is empty.";
        return;
    }
    izenelib::util::UString category;
    if (!doc.getProperty(config_.category_property_name, category))
    {
        error_ = "Category is empty.";
        return;
    }

    if (title.empty() || category.empty())
    {
        error_ = "Title or Category length equals zero.";
        return;
    }
    ProductPrice price;
    if (!util_->GetPrice(doc, price))
    {
        error_ = "Price is invalid.";
        return;
    }

    izenelib::util::UString city;
    doc.getProperty(config_.city_property_name, city);
    ProductClusteringAttach attach;
    attach.category = category;
    attach.city = city;
    attach.price = price;

    std::vector<std::pair<std::string, double> > doc_vector;
    analyzer_.Analyze(title, doc_vector);

    if (doc_vector.empty())
    {
        error_ = "Title analyzer result is empty.";
        return;
    }
    std::string docid;
    udocid.convertString(docid, izenelib::util::UString::UTF_8);
    dd_->InsertDoc(docid, doc_vector, attach);
}

bool ProductClustering::Run()
{
    bool run_dd = dd_->RunDdAnalysis();
    return run_dd;
}

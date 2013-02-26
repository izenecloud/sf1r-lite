#ifndef SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_
#define SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_

#include <query-manager/QueryTypeDef.h>
#include <mining-manager/group-manager/GroupParam.h>
#include "IncrementalFuzzyIndex.h"
#include "FilterManager.h"


using namespace sf1r::faceted;
namespace sf1r
{
class IncrementalFuzzyManager
{
public:
    IncrementalFuzzyManager(const std::string& path,
                        const std::string& tokenize_path,
                        const std::string& property,
                        boost::shared_ptr<DocumentManager>& document_manager,
                        boost::shared_ptr<IDManager>& idManager,
                        boost::shared_ptr<LAManager>& laManager,
                        IndexBundleSchema& indexSchema,
                        faceted::GroupManager* groupmanager,
                        faceted::AttrManager* attrmanager,
                        NumericPropertyTableBuilder* numeric_tablebuilder
                      );

    ~IncrementalFuzzyManager();

    //init;
    void Init();

    //addIndex
    void createIndex();
    void doCreateIndex();
    void prepare_index();

    //search
    bool fuzzySearch(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity
                    //, const SearchingMode::SuffixMatchFilterMode& filter_mode
                    , const std::vector<QueryFiltering::FilteringType>& filter_param
                    , const faceted::GroupParam& group_param);

    bool exactSearch(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity);

    //merge index;
    bool mergeIndexToFmIndex()
    {
        return false;

    }
    bool doAfterMergeIndexToFmIndex()
    {
        return false;
    }
    void reset();
    bool resetFilterManager()
    {
        return false;
    }
    bool resetIncFuzzyIndex()
    {
        return false;
    }

    //save and load index;
    bool saveLastDocid(std::string path = "");
    bool loadLastDocid(std::string path = "");
    void save();

    //other helper functions;
    void getDocNum(uint32_t& docNum);
    void getMaxNum(uint32_t& maxNum);
    void setLastDocid(uint32_t last_docid);
    void getLastDocid(uint32_t& last_docid);

    boost::shared_ptr<FilterManager>& getFilterManager()
    {
        return filter_manager_;
    }

private:
    //filter :

    bool getAllFilterRangeFromGroupLable(
            const faceted::GroupParam& group_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const;

    bool getAllFilterRangeFromFilterParam(
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const;

    void getAllFilterDocid(
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list,
            std::vector<uint32_t>& filterDocidList);

    bool indexForDoc(uint32_t& docId, std::string propertyString);

    void buildTokenizeDic();

    void delete_AllIndexFile();

private:
    uint32_t last_docid_;

    std::string index_path_;
    std::string tokenize_path_;
    std::string property_;
    //vector<std::string> SearchProperties;

    //vector<std::string>

    unsigned int BarrelNum_;

    IncrementalFuzzyIndex* pMainFuzzyIndexBarrel_;

    unsigned int IndexedDocNum_;

    bool isMergingIndex_;
    bool isInitIndex_;
    bool isAddingIndex_;//ok

    boost::shared_ptr<DocumentManager> document_manager_;

    boost::shared_ptr<IDManager> idManager_;

    boost::shared_ptr<LAManager> laManager_;

    IndexBundleSchema indexSchema_;

    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;
    mutable MutexType mutex_;

    boost::shared_ptr<FilterManager> filter_manager_;
};

}
#endif

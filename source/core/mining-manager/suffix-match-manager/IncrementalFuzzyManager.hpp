#ifndef SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_
#define SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_

#include <query-manager/QueryTypeDef.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <mining-manager/group-manager/DateStrFormat.h>
#include "IncrementalFuzzyIndex.h"
#include "FilterManager.h"

using namespace sf1r::faceted;
namespace{

    typedef std::pair<int64_t, int64_t> NumericRange;
    bool checkLabelParam_inc(const GroupParam::GroupLabelParam& labelParam, bool& isRange)
    {
        const GroupParam::GroupPathVec& labelPaths = labelParam.second;

        if (labelPaths.empty())
            return false;

        const GroupParam::GroupPath& path = labelPaths[0];
        if (path.empty())
            return false;

        const std::string& propValue = path[0];
        std::size_t delimitPos = propValue.find("-");
        isRange = (delimitPos != std::string::npos);

        return true;
    }

    bool convertNumericLabel_inc(const std::string& src, float& target)
    {
        std::size_t delimitPos = src.find("-");
        if (delimitPos != std::string::npos)
        {
            LOG(ERROR) << "group label parameter: " << src
                       << ", it should be specified as single numeric value";
            return false;
        }

        try
        {
            target = boost::lexical_cast<float>(src);
        }
        catch(const boost::bad_lexical_cast& e)
        {
            LOG(ERROR) << "failed in casting label from " << src
                       << " to numeric value, exception: " << e.what();
            return false;
        }

        return true;
    }

    bool convertRangeLabel_inc(const std::string& src, NumericRange& target)
    {
        std::size_t delimitPos = src.find("-");
        if (delimitPos == std::string::npos)
        {
            LOG(ERROR) << "group label parameter: " << src
                       << ", it should be specified as numeric range value";
            return false;
        }

        int64_t lowerBound = std::numeric_limits<int64_t>::min();
        int64_t upperBound = std::numeric_limits<int64_t>::max();

        try
        {
            if (delimitPos)
            {
                std::string sub = src.substr(0, delimitPos);
                lowerBound = boost::lexical_cast<int64_t>(sub);
            }

            if (delimitPos+1 != src.size())
            {
                std::string sub = src.substr(delimitPos+1);
                upperBound = boost::lexical_cast<int64_t>(sub);
            }
        }
        catch(const boost::bad_lexical_cast& e)
        {
            LOG(ERROR) << "failed in casting label from " << src
                        << " to numeric value, exception: " << e.what();
            return false;
        }

        target.first = lowerBound;
        target.second = upperBound;

        return true;
    }
}

namespace sf1r
{

class IncrementalFuzzyManager
{
public:
    IncrementalFuzzyManager(const std::string& path,
                        const std::string& tokenize_path,
                        const vector<std::string> properties,
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
                    , const std::vector<string>& search_in_properties
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity
                    //, const SearchingMode::SuffixMatchFilterMode& filter_mode
                    , const std::vector<QueryFiltering::FilteringType>& filter_param
                    , const faceted::GroupParam& group_param);

    bool exactSearch(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity);

    //only for cron-job : merge index;

    bool mergeIndexToFmIndex()
    {

        return false;
    }

    bool doAfterMergeIndexToFmIndex()
    {
        return false;
    }

    void reset();
    
    bool resetFilterManager();

    //save and load index;
    bool saveDocid(string property, uint32_t i);
    bool loadDocid(string property, uint32_t i);
    void save();

    //other helper functions;
    void getDocNum(uint32_t& docNum);
    void getMaxNum(uint32_t& maxNum);
    void setLastDocid(uint32_t last_docid);
    void getLastDocid(uint32_t& last_docid);

    void setStartDocid(uint32_t last_docid);
    void getStartDocid(uint32_t& last_docid);

    boost::shared_ptr<FilterManager>& getFilterManager()
    {
        return filter_manager_;
    }

    bool isEmpty()
    {
        bool isEmpty_ = true;
        for (unsigned int i = 0; i < IndexedDocNum_.size(); ++i)
        {
            if (IndexedDocNum_[0] != 0)
            {
                isEmpty_ = false;
                break;
            }
        }
        return isEmpty_;
    }

    void updateFilterForRtype();
    void printFilter();

private:

    bool buildGroupLabelDoc(docid_t docID, const Document& doc);

    bool getAllFilterRangeFromGroupLable(
            const faceted::GroupParam& group_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const;

    bool getAllFilterRangeFromFilterParam(
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const;

    bool getAllFilterRangeFromAttrLable(
            const faceted::GroupParam& group_param,
            std::vector<size_t>& prop_id_list,
            std::vector<FilterManager::FilterIdRange>& filter_range_list) const;

    void getAllFilterDocid(
            vector<std::vector<size_t> >& prop_id_lists
            ,std::vector<std::vector<FilterManager::FilterIdRange> >& filter_range_lists
            ,std::vector<uint32_t>& filterDocidList);

    void getResultByFilterDocid(
                std::vector<uint32_t>& filterDocidList
                , std::vector<uint32_t>& resultList_
                , std::vector<double> &ResultListSimilarity_
                , std::vector<uint32_t>& resultList
                , std::vector<double>& ResultListSimilarity);


    void getFinallyFilterDocid(std::vector<std::vector<uint32_t> >& filter_doc_lists
                            , std::vector<uint32_t>& filterDocidList);

    bool indexForDoc(uint32_t& docId, std::string propertyString, uint32_t i);

    void buildTokenizeDic();

    void delete_AllIndexFile();

private:
    vector<uint32_t> last_docid_;
    vector<uint32_t> start_docid_;

    std::string index_path_;
    std::string tokenize_path_;
    vector<std::string> properties_;

    std::map<std::string, unsigned int> properity_id_map_;

    unsigned int BarrelNum_;

    std::vector<IncrementalFuzzyIndex*> pMainFuzzyIndexBarrels_;
//    IncrementalFuzzyIndex* pMainFuzzyIndexBarrel_;

    vector<uint32_t> IndexedDocNum_;

    bool isMergingIndex_;
    bool isInitIndex_;
    bool isAddingIndex_;

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
    boost::shared_ptr<FilterManager> new_filter_manager;
};
}
#endif

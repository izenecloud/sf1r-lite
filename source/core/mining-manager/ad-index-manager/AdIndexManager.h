/*
 *  AdIndexManager.h
 */

#ifndef SF1_AD_INDEX_MANAGER_H_
#define SF1_AD_INDEX_MANAGER_H_

#include "AdMiningTask.h"
#include <boost/lexical_cast.hpp>
#include <common/PropSharedLockSet.h>
#include <search-manager/NumericPropertyTableBuilder.h>


#define CPM 0
#define CPC 1

namespace sf1r
{

class MiningTask;
class DocumentManager;
class AdMessage;
class AdClickPredictor;
class SearchKeywordOperation;
class KeywordSearchResult;
class SearchBase;
namespace faceted
{
    class GroupManager;
}

class AdIndexManager
{
public:
    AdIndexManager(
            const std::string& indexPath,
            const std::string& clickPredictorWorkingPath,
            boost::shared_ptr<DocumentManager>& dm,
            NumericPropertyTableBuilder* ntb,
            SearchBase* searcher,
            faceted::GroupManager* grp_mgr);

    ~AdIndexManager();

    bool buildMiningTask();

    inline AdMiningTask* getMiningTask()
    {
        return adMiningTask_;
    }

    void onAdStreamMessage(const std::vector<AdMessage>& msg_list);

    std::string getValueStrFromPropId(uint32_t pvid);

    void rankAndSelect(
        std::vector<std::pair<std::string, std::string> > userinfo,
        std::vector<docid_t>& docids,
        std::vector<float>& topKScoreRankList,
        std::size_t& totalCount);
    bool searchByQuery(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);
    bool searchByDNF(const std::vector<std::pair<std::string, std::string> >& info,
            std::vector<docid_t>& docids,
            std::vector<float>& topKRankScoreList,
            std::size_t& totalCount
            );
    void postMining(docid_t startid, docid_t endid);

private:

    std::string indexPath_;

    std::string clickPredictorWorkingPath_;
    std::string ad_selector_data_path_;

    boost::shared_ptr<DocumentManager>& documentManager_;
    AdMiningTask* adMiningTask_;
    AdClickPredictor* ad_click_predictor_;
    NumericPropertyTableBuilder* numericTableBuilder_;
    SearchBase* ad_searcher_;
    faceted::GroupManager* groupManager_;
};

} //namespace sf1r
#endif

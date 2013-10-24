/*
 *  AdIndexManager.h
 */

#ifndef SF1_AD_INDEX_MANAGER_H_
#define SF1_AD_INDEX_MANAGER_H_

#include "AdMiningTask.h"
#include "AdClickPredictor.h"
#include <boost/lexical_cast.hpp>
#include <common/PropSharedLockSet.h>
#include <search-manager/NumericPropertyTableBuilder.h>


#define CPM 0
#define CPC 1

namespace sf1r
{

class MiningTask;
class DocumentManager;

class AdIndexManager
{
public:
    AdIndexManager(
            const std::string& path,
            boost::shared_ptr<DocumentManager>& dm,
            NumericPropertyTableBuilder* ntb);

    ~AdIndexManager();

    bool buildMiningTask();

    inline AdMiningTask* getMiningTask()
    {
        return adMiningTask_;
    }

    bool search(const std::vector<std::pair<std::string, std::string> >& info,
            std::vector<docid_t>& docids,
            std::vector<float>& topKRankScoreList,
            std::size_t& totalCount
            );

private:

    std::string indexPath_;

    std::string clickPredictorWorkingPath_;

    boost::shared_ptr<DocumentManager>& documentManager_;

    AdMiningTask* adMiningTask_;

    NumericPropertyTableBuilder* numericTableBuilder_;
};

} //namespace sf1r
#endif

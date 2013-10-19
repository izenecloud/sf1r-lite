/*
 *  AdIndexManager.h
 */

#ifndef SF1_AD_INDEX_MANAGER_H_
#define SF1_AD_INDEX_MANAGER_H_

#include "AdMiningTask.h"
#include <boost/lexical_cast.hpp>

namespace sf1r
{

class MiningTask;
class DocumentManager;

class AdIndexManager
{
public:
    AdIndexManager(
            const std::string& path,
            boost::shared_ptr<DocumentManager>& dm);

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

    boost::shared_ptr<DocumentManager>& documentManager_;

    AdMiningTask* adMiningTask_;
};

} //namespace sf1r
#endif

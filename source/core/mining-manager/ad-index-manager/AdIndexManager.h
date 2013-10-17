/*
 *  AdIndexManager.h
 */

#ifndef SF1_AD_INDEX_MANAGER_H_
#define SF1_AD_INDEX_MANAGER_H_

#include "AdMiningTask.h"

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
            std::vector<docid_t>& docids);

private:

    std::string indexPath_;

    boost::shared_ptr<DocumentManager>& documentManager_;

    AdMiningTask* adMiningTask_;
};

} //namespace sf1r
#endif

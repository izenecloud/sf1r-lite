/*
 *   AdMiningTask.h
 */

#ifndef SF1_AD_MINING_TASK_H_
#define SF1_AD_MINING_TASK_H_

#include "../MiningTask.h"
#include <ir/be_index/DNFInvIndex.hpp>

#include <boost/shared_ptr.hpp>

namespace sf1r{

class DocumentManager;

class AdMiningTask : public MiningTask
{
public:
    AdMiningTask(
            const std::string& path,
            boost::shared_ptr<DocumentManager> dm);
    ~AdMiningTask();
    typedef izenelib::ir::be::DNFInvIndex AdIndexType;
    bool buildDocument(docid_t docID, const Document& doc);
    bool preProcess(int64_t timestamp);
    bool postProcess();

    inline docid_t getLastDocId()
    {
        return startDocId_;
    }

    void save();
    bool load();
private:
    std::string indexPath_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<AdIndexType> adIndex_;
    docid_t startDocId_;
};

} //namespace sf1r
#endif

/*
 *   AdMiningTask.h
 */

#ifndef SF1_AD_MINING_TASK_H_
#define SF1_AD_MINING_TASK_H_

#include "../MiningTask.h"
#include "DNFParser.h"
#include <ir/be_index/InvIndex.hpp>
#include <document-manager/DocumentManager.h>
#include <glog/logging.h>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <iostream>

using namespace std;

namespace sf1r{

class AdMiningTask : public MiningTask
{
public:
    AdMiningTask(
            const std::string& path,
            boost::shared_ptr<DocumentManager> dm);

    ~AdMiningTask();

    typedef izenelib::ir::be_index::DNFInvIndex AdIndexType;
   // typedef DNFInvIndex AdIndexType;

    bool buildDocument(docid_t docID, const Document& doc);

    bool preProcess(int64_t timestamp);

    bool postProcess();

    inline docid_t getLastDocId()
    {
        return startDocId_;
    }

    void retrieve(
            const std::vector<std::pair<std::string, std::string> >& info,
            boost::unordered_set<uint32_t>& dnfIDs)
    {
        adIndex_->retrieve(info, dnfIDs);
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

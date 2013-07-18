///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.29
///
#ifndef SUFFIX_MATCH_MININGTASK_H
#define SUFFIX_MATCH_MININGTASK_H

#include "../MiningTask.h"
#include "FMIndexManager.h"
#include <common/type_defs.h>
#include <am/succinct/fm-index/fm_index.hpp>
#include <query-manager/ActionItem.h>
#include <mining-manager/group-manager/GroupParam.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{

class DocumentManager;
class FilterManager;
class FMIndexManager;
class ProductTokenizer;
namespace faceted
{
    class GroupManager;
    class AttrManager;
}
class NumericPropertyTableBuilder;

class SuffixMatchMiningTask: public MiningTask
{
public:
    explicit SuffixMatchMiningTask(
            boost::shared_ptr<DocumentManager>& document_manager,
            boost::shared_ptr<FMIndexManager>& fmi_manager,
            boost::shared_ptr<FilterManager>& filter_manager,
            std::string data_root_path,
            ProductTokenizer* tokenizer,
            boost::shared_mutex& mutex);

    ~SuffixMatchMiningTask();

    bool buildDocument(docid_t docID, const Document& doc);
    bool preProcess();
    bool postProcess();
    docid_t getLastDocId();

    void setLastDocId(docid_t docID);
    void setTaskStatus(bool is_incrememtalTask);

    bool isRtypeIncremental_;

    const std::vector<double>& getDocumentScore();

    void saveDocumentScore();
    void loadDocumentScore();
    void clearDocumentScore();

private:
    DISALLOW_COPY_AND_ASSIGN(SuffixMatchMiningTask);
    boost::shared_ptr<DocumentManager> document_manager_;

    boost::shared_ptr<FMIndexManager>& fmi_manager_;
    boost::shared_ptr<FilterManager>& filter_manager_;

    std::string data_root_path_;

    bool is_incrememtalTask_;

    boost::shared_ptr<FMIndexManager> new_fmi_manager;
    boost::shared_ptr<FilterManager> new_filter_manager;
    bool need_rebuild;

    std::vector<double> document_Scores_;
    bool isDocumentScoreRebuild_;
    docid_t lastDocScoreDocid_;

    ProductTokenizer* tokenizer_;

    typedef boost::shared_mutex MutexType;
    MutexType& mutex_;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;
};
}

#endif

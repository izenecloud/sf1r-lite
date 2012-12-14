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
namespace faceted
{
    class GroupManager;
    class AttrManager;
}
class NumericPropertyTableBuilder;

class SuffixMatchMiningTask: public MiningTask
{
public:
    SuffixMatchMiningTask(
            boost::shared_ptr<DocumentManager>& document_manager,
            boost::shared_ptr<FMIndexManager>& fmi,
            boost::shared_ptr<FilterManager>& filter_manager,
            std::string data_root_path);

    ~SuffixMatchMiningTask();

    bool buildDocument(docid_t docID, const Document& doc);
    bool preProcess();
    bool postProcess();
    docid_t getLastDocId();

private:
    boost::shared_ptr<DocumentManager> document_manager_;

    boost::shared_ptr<FMIndexManager>& fmi_manager_;
    boost::shared_ptr<FilterManager>& filter_manager_;

    std::string data_root_path_;

    boost::shared_ptr<FMIndexManager> new_fmi_manager;
    boost::shared_ptr<FilterManager> new_filter_manager;
    bool need_rebuild;

    typedef boost::shared_mutex MutexType;
    mutable MutexType mutex_;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;
};
}

#endif

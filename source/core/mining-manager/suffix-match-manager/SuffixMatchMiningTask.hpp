///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.29
///
#ifndef SUFFIX_MATCH_MININGTASK_H
#define SUFFIX_MATCH_MININGTASK_H

#include "../MiningTask.h"


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
namespace faceted
{
    class GroupManager;
    class AttrManager;
}
class NumericPropertyTableBuilder;

class SuffixMatchMiningTask: public MiningTask
{
	typedef izenelib::am::succinct::fm_index::FMIndex<uint16_t> FMIndexType;

public:
	SuffixMatchMiningTask(boost::shared_ptr<DocumentManager> document_manager
			, std::vector<std::string>& group_property_list
    		, std::vector<std::string>& attr_property_list
    		, std::set<std::string>& number_property_list
			, boost::shared_ptr<FMIndexType>& fmi
    		, boost::shared_ptr<FilterManager>& filter_manager
    		, std::string data_root_path
    		, std::string fm_index_path
    		, std::string orig_text_path
    		, std::string property);

	~SuffixMatchMiningTask();

	bool buildDocment(docid_t docID, Document& doc);
	void preProcess();
	void postProcess();
	docid_t getLastDocId();

private:
	boost::shared_ptr<DocumentManager> document_manager_;

	std::vector<std::string>& group_property_list_;
    std::vector<std::string>& attr_property_list_;
    std::set<std::string>& number_property_list_;
	
	boost::shared_ptr<FMIndexType>& fmi_;
    boost::shared_ptr<FilterManager>& filter_manager_;

    std::string data_root_path_;
    std::string fm_index_path_;
    std::string orig_text_path_;

    std::string property_;
    FMIndexType* new_fmi_tmp;
    FilterManager* new_filter_manager;

	typedef boost::shared_mutex MutexType;
    mutable MutexType mutex_;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;
};
}
#endif
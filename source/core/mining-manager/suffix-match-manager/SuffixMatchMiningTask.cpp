#include "SuffixMatchMiningTask.hpp"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>
#include "FilterManager.h"
#include "FMIndexManager.h"
namespace sf1r
{

SuffixMatchMiningTask::SuffixMatchMiningTask(boost::shared_ptr<DocumentManager> document_manager
        , std::vector<std::string>& group_property_list
        , std::vector<std::string>& attr_property_list
        , std::set<std::string>& number_property_list
        , std::vector<std::string>& date_property_list
        , boost::shared_ptr<FMIndexManager>& fmi
        , boost::shared_ptr<FilterManager>& filter_manager
        , std::string data_root_path)
    : document_manager_(document_manager)
    , group_property_list_(group_property_list)
    , attr_property_list_(attr_property_list)
    , number_property_list_(number_property_list)
    , date_property_list_(date_property_list)
    , fmi_(fmi)
    , filter_manager_(filter_manager)
    , data_root_path_(data_root_path)
{
}

SuffixMatchMiningTask::~SuffixMatchMiningTask()
{

}

bool SuffixMatchMiningTask::preProcess()
{
    std::vector<FilterManager::StrFilterItemMapT> group_filter_map;
    std::vector<FilterManager::StrFilterItemMapT> attr_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> num_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> date_filter_map;
    
    new_filter_manager.reset(new FilterManager(filter_manager_->getGroupManager(), data_root_path_,
        filter_manager_->getAttrManager(), filter_manager_->getNumericTableBuilder()));
    new_filter_manager->setNumericAmp(filter_manager_->getNumericAmp());

    boost::shared_ptr<FMIndexManager> new_fmi_manager_tmp(new FMIndexManager(data_root_path_,
            document_manager_, new_filter_manager));
    new_fmi_manager = new_fmi_manager_tmp;

    std::vector<std::string> properties;
    for(int i = 0; i < FMIndexManager::LAST; ++i)
    {
        fmi_->getProperties(properties, (FMIndexManager::PropertyFMType)i);
        new_fmi_manager->addProperties(properties, (FMIndexManager::PropertyFMType)i);
        std::vector<std::string>().swap(properties);
    }

    size_t last_docid = fmi_ ? fmi_->docCount() : 0;
    is_need_rebuild = false;
    std::vector<uint32_t> del_docid_list;
    document_manager_->getDeletedDocIdList(del_docid_list);
    if(last_docid == document_manager_->getMaxDocId())
    {
        // check if there is any new deleted doc.
        std::vector<size_t> doclen_list(del_docid_list.size(), 0);
        fmi_->getDocLenList(del_docid_list, doclen_list);
        for(size_t i = 0; i < doclen_list.size(); ++i)
        {
            if(doclen_list[i] > 0)
            {
                is_need_rebuild = true;
                break;
            }
        }
    }
    else
    {
        LOG(INFO) << "old fmi docCount is : " << last_docid << ", document_manager count:" << document_manager_->getMaxDocId();
        is_need_rebuild = true;
    }
    if(is_need_rebuild)
        LOG(INFO) << "rebuilding in fm-index is needed.";
    else
    {
        new_fmi_manager->useOldDocCount(fmi_.get());
    }
    uint32_t max_group_docid = 0;
    uint32_t max_attr_docid = 0;
    if (last_docid)
    {
        LOG(INFO) << "start rebuilding in fm-index";
        if (new_filter_manager)
        {
            max_group_docid = new_filter_manager->loadStrFilterInvertedData(group_property_list_, group_filter_map);
            max_attr_docid = new_filter_manager->loadStrFilterInvertedData(attr_property_list_, attr_filter_map);
        }
    }

    LOG(INFO) << "building filter data in fm-index, start from:" << max_group_docid;
    std::vector<std::string > number_property_list(number_property_list_.begin(), number_property_list_.end());

    new_filter_manager->buildGroupFilterData(max_group_docid, document_manager_->getMaxDocId(),
                                                 group_property_list_, group_filter_map);
    new_filter_manager->saveStrFilterInvertedData(group_property_list_, group_filter_map);

    new_filter_manager->buildAttrFilterData(max_attr_docid, document_manager_->getMaxDocId(),
                                                attr_property_list_, attr_filter_map);
    new_filter_manager->saveStrFilterInvertedData(attr_property_list_, attr_filter_map);

    new_filter_manager->buildNumericFilterData(0, document_manager_->getMaxDocId(),
                                                  number_property_list, num_filter_map);
    new_filter_manager->buildDateFilterData(0, document_manager_->getMaxDocId(),
                                                date_property_list_, date_filter_map);
        
    new_fmi_manager->setFilterList(new_filter_manager->getFilterList());
    std::vector<FilterManager::StrFilterItemMapT>().swap(group_filter_map);
    std::vector<FilterManager::StrFilterItemMapT>().swap(attr_filter_map);
    std::vector<FilterManager::NumFilterItemMapT>().swap(num_filter_map);

    LOG(INFO) << "building filter data finished";
    bool isInitAndLoad = true;
    if(!new_fmi_manager->initAndLoadOldDocs(fmi_.get()))
    {
        LOG(ERROR) << "fmindex init building failed, must stop. ";
        new_fmi_manager->clearFMIData();
        isInitAndLoad = false;
    }
    if (is_need_rebuild && !isInitAndLoad)
        return false;
    return true;
}

bool SuffixMatchMiningTask::postProcess()
{
    if (!new_fmi_manager->buildCollectionAfter() && is_need_rebuild)
        return false;

    new_fmi_manager->buildExternalFilter();
    {
        WriteLock lock(mutex_);
        if(!is_need_rebuild)
        {
            // no rebuilding, so just take the owner of old data.
            LOG(INFO) << "no rebuild need, just swap data for common properties.";
            new_fmi_manager->swapCommonPropertiesData(fmi_.get());
        }
        fmi_.swap(new_fmi_manager);
        filter_manager_.swap(new_filter_manager);
        new_fmi_manager.reset();
        new_filter_manager.reset();
    }
    fmi_->saveAll();
    filter_manager_->saveFilterId();
    filter_manager_->clearFilterList();
    LOG(INFO) << "building fm-index finished";
    return true;
}

bool SuffixMatchMiningTask::buildDocment(docid_t docID, const Document& doc)
{
    bool failed = (doc.getId() == 0);
    new_fmi_manager->appendDocsAfter(failed, doc);
    return true;
}

docid_t SuffixMatchMiningTask::getLastDocId()
{
    return fmi_ ? fmi_->docCount()+1 : 1;
}

}

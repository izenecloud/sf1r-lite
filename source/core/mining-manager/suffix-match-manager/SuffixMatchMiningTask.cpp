#include "SuffixMatchMiningTask.hpp"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>
#include "FilterManager.h"
namespace sf1r
{

SuffixMatchMiningTask::SuffixMatchMiningTask(boost::shared_ptr<DocumentManager> document_manager
        , std::vector<std::string>& group_property_list
        , std::vector<std::string>& attr_property_list
        , std::set<std::string>& number_property_list
        , std::vector<std::string>& date_property_list
        , boost::shared_ptr<FMIndexType>& fmi
        , boost::shared_ptr<FilterManager>& filter_manager
        , std::string data_root_path
        , std::string fm_index_path
        , std::string orig_text_path
        , std::string property)
    : document_manager_(document_manager)
    , group_property_list_(group_property_list)
    , attr_property_list_(attr_property_list)
    , number_property_list_(number_property_list)
    , date_property_list_(date_property_list)
    , fmi_(fmi)
    , filter_manager_(filter_manager)
    , data_root_path_(data_root_path)
    , fm_index_path_(fm_index_path)
    , orig_text_path_(orig_text_path)
    , property_(property)
    , new_fmi_tmp(NULL)
    , new_filter_manager(NULL)
{
    new_fmi_tmp = new FMIndexType();
}

SuffixMatchMiningTask::~SuffixMatchMiningTask()
{
    if(new_filter_manager) delete new_filter_manager;
    if(new_fmi_tmp) delete new_fmi_tmp;
}

void SuffixMatchMiningTask::preProcess()
{
    std::vector<FilterManager::StrFilterItemMapT> group_filter_map;
    std::vector<FilterManager::StrFilterItemMapT> attr_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> num_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> date_filter_map;
    
    if (filter_manager_)
    {
        new_filter_manager = new FilterManager(filter_manager_->getGroupManager(), data_root_path_,
                                               filter_manager_->getAttrManager(), filter_manager_->getNumericTableBuilder());
        new_filter_manager->setNumericAmp(filter_manager_->getNumericAmp());
    }
    size_t last_docid = fmi_ ? fmi_->docCount() : 0;
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
        std::ifstream ifs(orig_text_path_.c_str());
        if (ifs)
        {
            new_fmi_tmp->loadOriginalText(ifs);
            std::vector<uint16_t> orig_text;
            new_fmi_tmp->swapOrigText(orig_text);
            std::vector<uint32_t> del_docid_list;
            document_manager_->getDeletedDocIdList(del_docid_list);
            fmi_->reconstructText(del_docid_list, orig_text);
            new_fmi_tmp->swapOrigText(orig_text);
        }
        else
        {
            last_docid = 0;
        }
    }

    LOG(INFO) << "building filter data in fm-index, start from:" << max_group_docid;
    std::vector<std::string > number_property_list(number_property_list_.begin(), number_property_list_.end());
    if (new_filter_manager)
    {
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
        
        new_fmi_tmp->setFilterList(new_filter_manager->getStringFilterList());
        new_fmi_tmp->setAuxFilterList(new_filter_manager->getNumericFilterList());
        
        std::vector<FilterManager::StrFilterItemMapT>().swap(group_filter_map);
        std::vector<FilterManager::StrFilterItemMapT>().swap(attr_filter_map);
        std::vector<FilterManager::NumFilterItemMapT>().swap(num_filter_map);
    }
    LOG(INFO) << "building filter data finished";
}

void SuffixMatchMiningTask::postProcess()
{
    std::ofstream ofs(orig_text_path_.c_str());
    new_fmi_tmp->saveOriginalText(ofs);
    ofs.close();

    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
    LOG(INFO) << "building fm-index....";
    new_fmi_tmp->build();

    {
        WriteLock lock(mutex_);
        fmi_.reset(new_fmi_tmp);
        filter_manager_.reset(new_filter_manager);
    }

    ofs.open(fm_index_path_.c_str());
    fmi_->save(ofs);
    filter_manager_->saveFilterId();
    filter_manager_->clearAllFilterLists();
    LOG(INFO) << "building fm-index finished";
}

bool SuffixMatchMiningTask::buildDocment(docid_t docID, Document& doc)
{
    if (doc.getId() == 0)
    {
        new_fmi_tmp->addDoc(NULL, 0);
        return true;
    }
    Document::property_const_iterator it = doc.findProperty(property_);
    if (it == doc.propertyEnd())
    {
       new_fmi_tmp->addDoc(NULL, 0);
       return true;
    }
    izenelib::util::UString text = it->second.get<UString>();
    Algorithm<UString>::to_lower(text);
    text = Algorithm<UString>::trim(text);
    new_fmi_tmp->addDoc(text.data(), text.length());
    return true;
}

docid_t SuffixMatchMiningTask::getLastDocId()
{
    return fmi_ ? fmi_->docCount()+1 : 1;
}

}

#include "group_manager.h"
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>

#include <iostream>

#include <glog/logging.h>

using namespace sf1r::faceted;

#define DEBUG_PRINT_GROUP 0
#define DEBUG_GROUPREP_FROM_DM 0

GroupManager::GroupManager(
    DocumentManager* documentManager,
    const std::string& dirPath
)
: documentManager_(documentManager)
, dirPath_(dirPath)
{
}

bool GroupManager::open(const std::vector<std::string>& properties)
{
    propValueMap_.clear();

    for (std::vector<std::string>::const_iterator it = properties.begin();
        it != properties.end(); ++it)
    {
        std::pair<PropValueMap::iterator, bool> res =
            propValueMap_.insert(PropValueMap::value_type(*it, PropValueTable(dirPath_, *it)));

        if (res.second)
        {
            if (!res.first->second.open())
            {
                LOG(ERROR) << "PropValueTable::open() failed, property name: " << res.first->first;
                return false;
            }
        }
        else
        {
            LOG(WARNING) << "the group property " << *it << " is opened already.";
        }
    }

    return true;
}

bool GroupManager::processCollection()
{
    LOG(INFO) << "start building group index data...";

    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }

    for (PropValueMap::iterator it = propValueMap_.begin();
        it != propValueMap_.end(); ++it)
    {
        const std::string& propName = it->first;
        PropValueTable& pvTable = it->second;
        std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();
        assert(! idTable.empty() && "id 0 should have been reserved in PropValueTable constructor");

        const docid_t maxDocId = documentManager_->getMaxDocId();
        idTable.reserve(maxDocId + 1);

        for (docid_t docId = idTable.size(); docId <= maxDocId; ++docId)
        {
            Document doc;
            PropValueTable::pvid_t pvId = 0;

            if (documentManager_->getDocument(docId, doc))
            {
                Document::property_iterator it = doc.findProperty(propName);
                if (it != doc.propertyEnd())
                {
                    const izenelib::util::UString& value = it->second.get<izenelib::util::UString>();
                    pvId = pvTable.propValueId(value);
                }
                else
                {
                    LOG(WARNING) << "Document::findProperty, doc id " << docId
                                 << " has no value on property " << propName;
                }
            }
            else
            {
                LOG(ERROR) << "DocumentManager::getDocument() failed, doc id: " << docId;
            }

            idTable.push_back(pvId);
        }

        if (!pvTable.flush())
        {
            LOG(ERROR) << "PropValueTable::flush() failed, property name: " << propName;
        }
    }

    LOG(INFO) << "finished building group index data";
    return true;
}

bool GroupManager::getGroupRep(
    const std::vector<unsigned int>& docIdList,
    const std::vector<std::string>& groupPropertyList,
    faceted::OntologyRep& groupRep
)
{
#if DEBUG_GROUPREP_FROM_DM
    return getGroupRepFromDocumentManager(docIdList, groupPropertyList, groupRep);
#else
    return getGroupRepFromTable(docIdList, groupPropertyList, groupRep);
#endif
}

bool GroupManager::getGroupRepFromDocumentManager(
    const std::vector<unsigned int>& docIdList,
    const std::vector<std::string>& groupPropertyList,
    faceted::OntologyRep& groupRep
)
{
    typedef std::vector<docid_t> DocIdList;
    typedef std::map<izenelib::util::UString, DocIdList> DocIdMap;
    typedef std::map<std::string, DocIdMap> PropertyMap;
    PropertyMap propertyMap;

    for (std::vector<unsigned int>::const_iterator docIt = docIdList.begin();
        docIt != docIdList.end(); ++docIt)
    {
#if DEBUG_PRINT_GROUP
        std::cout << "docid: " << *docIt << std::endl;
#endif

        Document doc;
        if (documentManager_->getDocument(*docIt, doc))
        {
            for (std::vector<std::string>::const_iterator propIt = groupPropertyList.begin();
                propIt != groupPropertyList.end(); ++propIt)
            {
                Document::property_iterator it = doc.findProperty(*propIt);
                if (it != doc.propertyEnd())
                {
                    const izenelib::util::UString& value = it->second.get<izenelib::util::UString>();
                    propertyMap[*propIt][value].push_back(*docIt);

#if DEBUG_PRINT_GROUP
                    std::string utf8Str;
                    value.convertString(utf8Str, izenelib::util::UString::UTF_8);
                    std::cout << *propIt << " value: " << utf8Str
                              << ", doc count: " << propertyMap[*propIt][value].size() << std::endl;
#endif
                }
                else
                {
                    LOG(WARNING) << "GroupManager::getGroupRepFromDocumentManager: doc id " << *docIt
                                 << " has no value on property " << *propIt;
                }
            }
        }
        else
        {
            LOG(ERROR) << "GroupManager::getGroupRepFromDocumentManager calls DocumentManager::getDocument() failed, doc id: " << *docIt;
            return false;
        }

#if DEBUG_PRINT_GROUP
        std::cout << std::endl;
#endif
    }

    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;
    for (std::vector<std::string>::const_iterator nameIt = groupPropertyList.begin();
        nameIt != groupPropertyList.end(); ++nameIt)
    {
        itemList.push_back(sf1r::faceted::OntologyRepItem());
        sf1r::faceted::OntologyRepItem& propItem = itemList.back();
        propItem.text.assign(*nameIt, izenelib::util::UString::UTF_8);

        PropertyMap::iterator propMapIt = propertyMap.find(*nameIt);
        if (propMapIt != propertyMap.end())
        {
            DocIdMap& docIdMap = propMapIt->second;
            for (DocIdMap::iterator docListIt = docIdMap.begin();
                    docListIt != docIdMap.end(); ++docListIt)
            {
                itemList.push_back(sf1r::faceted::OntologyRepItem());
                sf1r::faceted::OntologyRepItem& valueItem = itemList.back();
                valueItem.level = 1;
                valueItem.text = docListIt->first;
                valueItem.doc_id_list.swap(docListIt->second);
                valueItem.doc_count = valueItem.doc_id_list.size();

                propItem.doc_count += valueItem.doc_count;
            }
        }
    }

    return true;
}

bool GroupManager::getGroupRepFromTable(
    const std::vector<unsigned int>& docIdList,
    const std::vector<std::string>& groupPropertyList,
    faceted::OntologyRep& groupRep
)
{
    typedef std::vector<docid_t> DocIdList;
    typedef std::map<PropValueTable::pvid_t, DocIdList> DocIdMap;

    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;

    for (std::vector<std::string>::const_iterator nameIt = groupPropertyList.begin();
        nameIt != groupPropertyList.end(); ++nameIt)
    {
        PropValueMap::const_iterator pvIt = propValueMap_.find(*nameIt);
        if (pvIt == propValueMap_.end())
        {
            LOG(ERROR) << "in GroupManager::getGroupRepFromTable: group index file is not loaded for property " << *nameIt;
            return false;
        }

        const PropValueTable& pvTable = pvIt->second;
        const std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();
        DocIdMap docIdMap;

        for (std::vector<unsigned int>::const_iterator docIt = docIdList.begin();
            docIt != docIdList.end(); ++docIt)
        {
            docid_t docId = *docIt;
            if (docId < idTable.size())
            {
                PropValueTable::pvid_t pvId = idTable[docId];
                if (pvId != 0)
                {
                    docIdMap[pvId].push_back(docId);
                }
            }
        }

        itemList.push_back(sf1r::faceted::OntologyRepItem());
        sf1r::faceted::OntologyRepItem& propItem = itemList.back();
        propItem.text.assign(*nameIt, izenelib::util::UString::UTF_8);

        for (DocIdMap::iterator docListIt = docIdMap.begin();
            docListIt != docIdMap.end(); ++docListIt)
        {
            itemList.push_back(sf1r::faceted::OntologyRepItem());
            sf1r::faceted::OntologyRepItem& valueItem = itemList.back();
            valueItem.level = 1;
            valueItem.text = pvTable.propValueStr(docListIt->first);
            valueItem.doc_id_list.swap(docListIt->second);
            valueItem.doc_count = valueItem.doc_id_list.size();

            propItem.doc_count += valueItem.doc_count;
        }
    }

    return true;
}

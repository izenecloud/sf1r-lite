#include "group_manager.h"
#include <mining-manager/util/FSUtil.hpp>
#include <document-manager/DocumentManager.h>
#include <mining-manager/MiningException.hpp>

#include <iostream>

#include <glog/logging.h>

using namespace sf1r::faceted;

#define DEBUG_PRINT_GROUP 0
#define DEBUG_GROUPREP_FROM_DM 0

namespace
{

/**
 * Check whether @p v1 is the parent node of @p v2.
 * @param parentTable table containing parent node for each node, 0 for root node
 * @param v1 node 1
 * @param v2 node 2
 * @return true for @p v1 is the parent node of @p v2, otherwise false is returned,
 *         when @p v1 equals to @p v2, true is returned,
 *         when either @p v1 or @p v2 is zero for invalid id, false is returned.
 */
bool isParentValue(
    const std::vector<PropValueTable::pvid_t>& parentTable,
    PropValueTable::pvid_t v1,
    PropValueTable::pvid_t v2
)
{
    if (v1 == 0 || v2 == 0)
        return false;

    if (v2 >= parentTable.size())
        return (v2 == v1);

    while (v2 != 0)
    {
        if (v2 == v1)
            return true;

        v2 = parentTable[v2];
    }

    return false;
}

}

GroupManager::GroupManager(
    DocumentManager* documentManager,
    const std::string& dirPath
)
: documentManager_(documentManager)
, dirPath_(dirPath)
{
}

bool GroupManager::open(const std::vector<GroupConfig>& configVec)
{
    propValueMap_.clear();

    for (std::vector<GroupConfig>::const_iterator it = configVec.begin();
        it != configVec.end(); ++it)
    {
        std::pair<PropValueMap::iterator, bool> res =
            propValueMap_.insert(PropValueMap::value_type(it->propName, PropValueTable(dirPath_, it->propName)));

        if (res.second)
        {
            PropValueTable& pvTable = res.first->second;
            if (!pvTable.open())
            {
                LOG(ERROR) << "PropValueTable::open() failed, property name: " << it->propName;
                return false;
            }

            if (!pvTable.setValueTree(it->valueTree))
            {
                LOG(ERROR) << "PropValueTable::setValueTree() failed, property name: " << it->propName;
                return false;
            }
        }
        else
        {
            LOG(WARNING) << "the group property " << it->propName << " is opened already.";
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

        const docid_t startDocId = idTable.size();
        const docid_t endDocId = documentManager_->getMaxDocId();
        idTable.reserve(endDocId + 1);

        LOG(INFO) << "start building property: " << propName
                  << ", start doc id: " << startDocId
                  << ", end doc id: " << endDocId;

        for (docid_t docId = startDocId; docId <= endDocId; ++docId)
        {
            Document doc;
            PropValueTable::pvid_t pvId = 0;

            if (documentManager_->getDocument(docId, doc))
            {
                Document::property_iterator it = doc.findProperty(propName);
                if (it != doc.propertyEnd())
                {
                    const izenelib::util::UString& value = it->second.get<izenelib::util::UString>();
                    try
                    {
                        pvId = pvTable.propValueId(value);
                    }
                    catch(MiningException& e)
                    {
                        LOG(ERROR) << "exception: " << e.what()
                                   << ", doc id: " << docId;
                    }
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

            if (docId % 100000 == 0)
            {
                LOG(INFO) << "inserted doc id: " << docId;
            }
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
    const std::vector<std::pair<std::string, std::string> >& groupLabelList,
    faceted::OntologyRep& groupRep
)
{
#if DEBUG_GROUPREP_FROM_DM
    return getGroupRepFromDocumentManager(docIdList, groupPropertyList, groupLabelList, groupRep);
#else
    return getGroupRepFromTable(docIdList, groupPropertyList, groupLabelList, groupRep);
#endif
}

bool GroupManager::getGroupRepFromDocumentManager(
    const std::vector<unsigned int>& docIdList,
    const std::vector<std::string>& groupPropertyList,
    const std::vector<std::pair<std::string, std::string> >& groupLabelList,
    faceted::OntologyRep& groupRep
)
{
    typedef std::vector<docid_t> DocIdList;
    typedef std::map<izenelib::util::UString, DocIdList> DocIdMap;
    typedef std::map<std::string, DocIdMap> PropertyMap;
    PropertyMap propertyMap;

    // condition list, each is a pair of property name and value
    typedef std::vector<std::pair<std::string, izenelib::util::UString> > CondList;
    CondList condList;
    condList.resize(groupLabelList.size());
    for (std::size_t i = 0; i < groupLabelList.size(); ++i)
    {
        condList[i].first = groupLabelList[i].first;
        condList[i].second.assign(groupLabelList[i].second, izenelib::util::UString::UTF_8);
    }

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
                Document::property_iterator docPropIt = doc.findProperty(*propIt);
                if (docPropIt != doc.propertyEnd())
                {
                    bool isMeetCond = true;
                    for (CondList::const_iterator condIt = condList.begin();
                        condIt != condList.end(); ++condIt)
                    {
                        if (condIt->first != *propIt)
                        {
                            Document::property_iterator condPropIt = doc.findProperty(condIt->first);
                            if (condPropIt != doc.propertyEnd()
                               && condPropIt->second.get<izenelib::util::UString>() != condIt->second)
                            {
                                isMeetCond = false;
                                break;
                            }
                        }
                    }

                    if (isMeetCond)
                    {
                        const izenelib::util::UString& value = docPropIt->second.get<izenelib::util::UString>();
                        propertyMap[*propIt][value].push_back(*docIt);
                    }

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
    const std::vector<std::pair<std::string, std::string> >& groupLabelList,
    faceted::OntologyRep& groupRep
)
{
    typedef std::vector<docid_t> DocIdList;
    typedef std::map<PropValueTable::pvid_t, DocIdList> DocIdMap;

    // a condition pair of value table and target value id
    typedef std::pair<const PropValueTable*, PropValueTable::pvid_t> CondPair;
    typedef std::vector<CondPair> CondList;
    CondList condList;
    for (std::size_t i = 0; i < groupLabelList.size(); ++i)
    {
        PropValueMap::const_iterator pvIt = propValueMap_.find(groupLabelList[i].first);
        if (pvIt == propValueMap_.end())
        {
            LOG(ERROR) << "in GroupManager::getGroupRepFromTable: group index file is not loaded for label property " << groupLabelList[i].first;
            return false;
        }

        const PropValueTable& pvTable = pvIt->second;
        izenelib::util::UString ustr(groupLabelList[i].second, izenelib::util::UString::UTF_8);
        PropValueTable::pvid_t pvId = pvTable.propValueId(ustr);
        condList.push_back(CondPair(&pvTable, pvId));
    }

    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;

    for (std::vector<std::string>::const_iterator nameIt = groupPropertyList.begin();
        nameIt != groupPropertyList.end(); ++nameIt)
    {
        PropValueMap::const_iterator pvIt = propValueMap_.find(*nameIt);
        if (pvIt == propValueMap_.end())
        {
            LOG(ERROR) << "in GroupManager::getGroupRepFromTable: group index file is not loaded for group property " << *nameIt;
            return false;
        }

        const PropValueTable& pvTable = pvIt->second;
        const std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();
        const std::vector<PropValueTable::pvid_t>& parentTable = pvTable.parentIdTable();
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
                    bool isMeetCond = true;
                    for (CondList::const_iterator condIt = condList.begin();
                        condIt != condList.end(); ++condIt)
                    {
                        const PropValueTable* condPVTable = condIt->first;
                        if (*nameIt != condPVTable->propName())
                        {
                            const std::vector<PropValueTable::pvid_t>& condIdTable = condPVTable->propIdTable();
                            if (docId < condIdTable.size()
                                && isParentValue(condPVTable->parentIdTable(), condIt->second, condIdTable[docId]) == false)
                            {
                                isMeetCond = false;
                                break;
                            }
                        }
                    }

                    if (isMeetCond)
                    {
                        docIdMap[pvId].push_back(docId);

                        if (pvId < parentTable.size())
                        {
                            pvId = parentTable[pvId];
                            while (pvId != 0)
                            {
                                docIdMap[pvId].push_back(docId);
                                pvId = parentTable[pvId];
                            }
                        }
                    }
                }
            }
        }

        // property name as root node
        itemList.push_back(sf1r::faceted::OntologyRepItem());
        sf1r::faceted::OntologyRepItem& propItem = itemList.back();
        propItem.text.assign(*nameIt, izenelib::util::UString::UTF_8);

        // nodes at configured level
        const std::list<OntologyRepItem>& treeList = pvTable.valueTree().item_list;
        for (std::list<OntologyRepItem>::const_iterator it = treeList.begin();
            it != treeList.end(); ++it)
        {
            DocIdMap::iterator mapIt = docIdMap.find(it->id);
            if (mapIt != docIdMap.end())
            {
                itemList.push_back(*it);
                faceted::OntologyRepItem& valueItem = itemList.back();
                valueItem.doc_id_list.swap(mapIt->second);
                valueItem.doc_count = valueItem.doc_id_list.size();

                if (valueItem.level == 1)
                {
                    propItem.doc_count += valueItem.doc_count;
                }

                docIdMap.erase(mapIt);
            }
        }

        // other nodes are appended as level 1
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

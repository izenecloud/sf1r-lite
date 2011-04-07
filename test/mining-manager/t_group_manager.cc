///
/// @file t_group_manager.cpp
/// @brief test getting doc list for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-23
///

#include <util/ustring/UString.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <mining-manager/faceted-submanager/group_manager.h>

#include <stdlib.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* PROP_NAME_GROUP_STR = "Group_str";
const char* PROP_NAME_GROUP_INT = "Group_int";
const char* PROP_NAME_GROUP_FLOAT = "Group_float";
const char* TEST_DIR_STR = "group_test";

struct DocInput
{
    unsigned int docId_;
    string title_;
    string groupStr_;
    string groupInt_;
    string groupFloat_;

    DocInput()
    : docId_(0)
    {
    }

    DocInput(
        unsigned int docId,
        const string& title,
        const string& groupStr,
        const string& groupInt,
        const string& groupFloat
    )
    : docId_(docId)
    , title_(title)
    , groupStr_(groupStr)
    , groupInt_(groupInt)
    , groupFloat_(groupFloat)
    {
    }

    DocInput(
        unsigned int docId,
        const string& groupStr,
        const string& groupInt,
        const string& groupFloat
    )
    : docId_(docId)
    , groupStr_(groupStr)
    , groupInt_(groupInt)
    , groupFloat_(groupFloat)
    {
        ostringstream oss;
        oss << "Title " << docId_;
        title_ = oss.str();
    }
};

typedef vector<unsigned int> DocIdList;
typedef map<string, DocIdList> DocIdMap; // key: property value
typedef map<string, DocIdMap> PropertyMap; // key: property name

void createDocInput(
    vector<DocInput>& docInputVec,
    int start,
    int end
)
{
    for (int i = start; i <= end; ++i)
    {
        DocInput docInput;
        int mod = i % 4;
        switch (mod)
        {
        case 0:
            docInput = DocInput(i, "aaa", "1", "0.1");
            break;
        case 1:
            docInput = DocInput(i, "上海", "2", "0.2");
            break;
        case 2:
            docInput = DocInput(i, "中国", "3", "0.3");
            break;
        case 3:
            docInput = DocInput(i, "aaa", "2", "0.3");
            break;
        default:
            BOOST_ASSERT(false);
        }

        docInputVec.push_back(docInput);
    }
}

void createPropertyMap(
    const vector<DocInput>& docInputVec,
    const std::vector<std::pair<std::string, std::string> >& labelList,
    PropertyMap& propertyMap
)
{
    DocIdMap& strDocIdMap = propertyMap[PROP_NAME_GROUP_STR];
    DocIdMap& intDocIdMap = propertyMap[PROP_NAME_GROUP_INT];
    DocIdMap& floatDocIdMap = propertyMap[PROP_NAME_GROUP_FLOAT];

    for (vector<DocInput>::const_iterator it = docInputVec.begin();
        it != docInputVec.end(); ++it)
    {
        bool isOK = true;
        for (std::size_t i = 0; i < labelList.size(); ++i)
        {
            if ((labelList[i].first == PROP_NAME_GROUP_INT && labelList[i].second != it->groupInt_)
                || (labelList[i].first == PROP_NAME_GROUP_FLOAT && labelList[i].second != it->groupFloat_))
            {
                isOK = false;
                break;
            }
        }
        if (isOK)
        {
            strDocIdMap[it->groupStr_].push_back(it->docId_);
        }

        isOK = true;
        for (std::size_t i = 0; i < labelList.size(); ++i)
        {
            if ((labelList[i].first == PROP_NAME_GROUP_STR && labelList[i].second != it->groupStr_)
                || (labelList[i].first == PROP_NAME_GROUP_FLOAT && labelList[i].second != it->groupFloat_))
            {
                isOK = false;
                break;
            }
        }
        if (isOK)
        {
            intDocIdMap[it->groupInt_].push_back(it->docId_);
        }

        isOK = true;
        for (std::size_t i = 0; i < labelList.size(); ++i)
        {
            if ((labelList[i].first == PROP_NAME_GROUP_STR && labelList[i].second != it->groupStr_)
                || (labelList[i].first == PROP_NAME_GROUP_INT && labelList[i].second != it->groupInt_))
            {
                isOK = false;
                break;
            }
        }
        if (isOK)
        {
            floatDocIdMap[it->groupFloat_].push_back(it->docId_);
        }

    }
}

void checkGroupRep(
    const faceted::OntologyRep& groupRep,
    const vector<string>& propNameList,
    PropertyMap& propertyMap
)
{
    typedef list<faceted::OntologyRepItem> RepItemList;
    const RepItemList& itemList = groupRep.item_list;

    // check list size
    unsigned int totalCount = 0;
    // iterate property name
    for (PropertyMap::const_iterator it = propertyMap.begin();
        it != propertyMap.end(); ++it)
    {
        ++totalCount;
        // get property value count
        totalCount += it->second.size();
    }
    BOOST_CHECK_EQUAL(itemList.size(), totalCount);

    string propName, convertBuffer;
    vector<string>::const_iterator propNameIt = propNameList.begin();
    for (RepItemList::const_iterator it = itemList.begin();
            it != itemList.end(); ++it)
    {
        const faceted::OntologyRepItem& item = *it;
        item.text.convertString(convertBuffer, ENCODING_TYPE);

        if (item.level == 0)
        {
            propName = convertBuffer;
            BOOST_CHECK(propNameIt != propNameList.end());
            BOOST_CHECK_EQUAL(propName, *propNameIt);
            ++propNameIt;
        }
        else
        {
            BOOST_CHECK_EQUAL(item.level, 1);

            DocIdList& docIdList = propertyMap[propName][convertBuffer];
            BOOST_TEST_MESSAGE("check property: " << propName
                               << ", value: " << convertBuffer
                               << ", doc count: " << docIdList.size());
            BOOST_CHECK_EQUAL(item.doc_count, docIdList.size());
            BOOST_CHECK_EQUAL_COLLECTIONS(item.doc_id_list.begin(), item.doc_id_list.end(),
                                          docIdList.begin(), docIdList.end());
        }
    }
}

void makeSchema(std::set<PropertyConfig, PropertyComp>& propertyConfig)
{
    PropertyConfig pconfig1;
    pconfig1.setName("DOCID");
    pconfig1.setType(sf1r::STRING_PROPERTY_TYPE);

    PropertyConfig pconfig2;
    pconfig2.setName("Title");
    pconfig2.setType(sf1r::STRING_PROPERTY_TYPE);

    PropertyConfig pconfig3;
    pconfig3.setName(PROP_NAME_GROUP_STR);
    pconfig3.setType(sf1r::STRING_PROPERTY_TYPE);

    PropertyConfig pconfig4;
    pconfig4.setName(PROP_NAME_GROUP_INT);
    pconfig3.setType(sf1r::INT_PROPERTY_TYPE);

    PropertyConfig pconfig5;
    pconfig5.setName(PROP_NAME_GROUP_FLOAT);
    pconfig3.setType(sf1r::FLOAT_PROPERTY_TYPE);

    propertyConfig.insert(pconfig1);
    propertyConfig.insert(pconfig2);
    propertyConfig.insert(pconfig3);
    propertyConfig.insert(pconfig4);
    propertyConfig.insert(pconfig5);
}

DocumentManager*
createDocumentManager()
{
    boost::filesystem::remove_all(TEST_DIR_STR);

    bfs::path dmPath(bfs::path(TEST_DIR_STR) / "dm/");
    bfs::create_directories(dmPath);
    std::set<PropertyConfig, PropertyComp> propertyConfig;
    makeSchema(propertyConfig);

    DocumentManager* ret = new DocumentManager(
            dmPath.string(),
            propertyConfig,
            ENCODING_TYPE,
            2000
            );
    return ret;
}

void prepareDocument(
    Document& document,
    const DocInput& docInput
)
{
    document.setId(docInput.docId_);

    ostringstream oss;
    oss << docInput.docId_;

    izenelib::util::UString property;
    property.assign(oss.str(), ENCODING_TYPE);
    document.property("DOCID") = property;

    property.assign(docInput.title_, ENCODING_TYPE);
    document.property("Title") = property;

    property.assign(docInput.groupStr_, ENCODING_TYPE);
    document.property(PROP_NAME_GROUP_STR) = property;

    property.assign(docInput.groupInt_, ENCODING_TYPE);
    document.property(PROP_NAME_GROUP_INT) = property;

    property.assign(docInput.groupFloat_, ENCODING_TYPE);
    document.property(PROP_NAME_GROUP_FLOAT) = property;
}

void checkProperty(
    const Document& doc,
    const string& propName,
    const string& propValue
)
{
    Document::property_const_iterator it = doc.findProperty(propName);
    BOOST_REQUIRE(it != doc.propertyEnd());

    const izenelib::util::UString& value = it->second.get<izenelib::util::UString>();
    std::string utf8Str;
    value.convertString(utf8Str, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL(utf8Str, propValue);
}

void checkDocument(
    Document& doc,
    const DocInput& docInput
)
{
    ostringstream oss;
    oss << docInput.docId_;
    checkProperty(doc, "DOCID", oss.str());
    checkProperty(doc, PROP_NAME_GROUP_STR, docInput.groupStr_);
    checkProperty(doc, PROP_NAME_GROUP_INT, docInput.groupInt_);
    checkProperty(doc, PROP_NAME_GROUP_FLOAT, docInput.groupFloat_);
}

void createCollection(
    DocumentManager* documentManager,
    const vector<DocInput>& docInputVec
)
{
    for (vector<DocInput>::const_iterator it = docInputVec.begin();
        it != docInputVec.end(); ++it)
    {
        Document document;
        prepareDocument(document, *it);
        BOOST_CHECK(documentManager->insertDocument(document));
    }
}

void checkCollection(
    DocumentManager* documentManager,
    const vector<DocInput>& docInputVec
)
{
    for (vector<DocInput>::const_iterator it = docInputVec.begin();
        it != docInputVec.end(); ++it)
    {
        Document doc;
        documentManager->getDocument(it->docId_, doc);
        checkDocument(doc, *it);
    }
}

void checkGroupManager(
    DocumentManager* documentManager,
    const string& groupPath,
    const vector<string>& propNameVec,
    const vector<DocInput>& docInputVec,
    bool isProcessCollection
)
{
    faceted::GroupManager* groupManager = new faceted::GroupManager(documentManager, groupPath);

    BOOST_CHECK(groupManager->open(propNameVec));

    if (isProcessCollection)
    {
        BOOST_CHECK(groupManager->processCollection());
    }

    vector<unsigned int> docIdList;
    for (vector<DocInput>::const_iterator it = docInputVec.begin();
            it != docInputVec.end(); ++it)
    {
        docIdList.push_back(it->docId_);
    }

    
    vector<string> getPropList(propNameVec);
    std::vector<std::pair<std::string, std::string> > labelList;
    BOOST_TEST_MESSAGE("check label size: " << labelList.size());
    faceted::OntologyRep groupRep;
    BOOST_CHECK(groupManager->getGroupRep(docIdList, getPropList, labelList, groupRep));

    PropertyMap propMap;
    createPropertyMap(docInputVec, labelList, propMap);
    checkGroupRep(groupRep, getPropList, propMap);


    labelList.push_back(std::pair<std::string, std::string>(PROP_NAME_GROUP_STR, "aaa"));
    labelList.push_back(std::pair<std::string, std::string>(PROP_NAME_GROUP_INT, "2"));
    BOOST_TEST_MESSAGE("check label size: " << labelList.size());
    groupRep.item_list.clear();
    BOOST_CHECK(groupManager->getGroupRep(docIdList, getPropList, labelList, groupRep));

    propMap.clear();
    createPropertyMap(docInputVec, labelList, propMap);
    checkGroupRep(groupRep, getPropList, propMap);

    delete groupManager;
}

}

BOOST_AUTO_TEST_SUITE(GroupManager_test)

BOOST_AUTO_TEST_CASE(getGroupRep)
{
    DocumentManager* documentManager = createDocumentManager();
    BOOST_REQUIRE(documentManager != NULL);

    vector<string> propNameVec;
    propNameVec.push_back(PROP_NAME_GROUP_STR);
    propNameVec.push_back(PROP_NAME_GROUP_INT);
    propNameVec.push_back(PROP_NAME_GROUP_FLOAT);

    bfs::path groupPath(bfs::path(TEST_DIR_STR) / "group");
    vector<DocInput> docInputVec;

    BOOST_TEST_MESSAGE("check empty group index");
    checkGroupManager(documentManager, groupPath.string(), propNameVec, docInputVec, false);

    createDocInput(docInputVec, 1, 100);
    createCollection(documentManager, docInputVec);
    checkCollection(documentManager, docInputVec);

    BOOST_TEST_MESSAGE("create group index 1st time");
    checkGroupManager(documentManager, groupPath.string(), propNameVec, docInputVec, true);

    BOOST_TEST_MESSAGE("load group index");
    checkGroupManager(documentManager, groupPath.string(), propNameVec, docInputVec, false);

    vector<DocInput> newDocInputVec;
    createDocInput(newDocInputVec, 101, 200);
    createCollection(documentManager, newDocInputVec);
    checkCollection(documentManager, newDocInputVec);

    BOOST_TEST_MESSAGE("create group index 2nd time");
    docInputVec.insert(docInputVec.end(), newDocInputVec.begin(), newDocInputVec.end());
    checkGroupManager(documentManager, groupPath.string(), propNameVec, docInputVec, true);

    delete documentManager;
}

BOOST_AUTO_TEST_SUITE_END() 

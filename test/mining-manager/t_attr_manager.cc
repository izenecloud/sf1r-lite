///
/// @file t_attr_manager.cpp
/// @brief test getting doc list for each attribute value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-23
///

#include <util/ustring/UString.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <mining-manager/faceted-submanager/attr_manager.h>

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

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* PROP_NAME_ATTR = "Attribute";
const char* TEST_DIR_STR = "attr_test";

struct DocInput
{
    unsigned int docId_;
    string title_;
    string attrStr_;

    DocInput()
    : docId_(0)
    {
    }

    DocInput(
        unsigned int docId,
        const string& attrStr
    )
    : docId_(docId)
    , attrStr_(attrStr)
    {
        ostringstream oss;
        oss << "Title " << docId_;
        title_ = oss.str();
    }
};

typedef vector<unsigned int> DocIdList;
struct ValueMap
{
    ValueMap(): docCount_(0) {}

    map<string, DocIdList> docIdMap_; // key: attribute value
    int docCount_;
};
typedef map<string, ValueMap> AttrMap; // key: attribute name

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
            docInput = DocInput(i, "品牌:Two In One/欧艾尼,领子:圆领");
            break;
        case 1:
            docInput = DocInput(i, "品牌:阿依莲,季节:春季|夏季|秋季,价格:2011");
            break;
        case 2:
            docInput = DocInput(i, "");
            break;
        case 3:
            docInput = DocInput(i, "品牌:淑女屋,季节:夏季,年份:2011,尺码:S|M|L|XL");
            break;
        default:
            BOOST_ASSERT(false);
        }

        docInputVec.push_back(docInput);
    }
}

void createAttrMap(
    const vector<DocInput>& docInputVec,
    AttrMap& attrMap
)
{
    for (vector<DocInput>::const_iterator it = docInputVec.begin();
        it != docInputVec.end(); ++it)
    {
        int mod = it->docId_ % 4;
        switch (mod)
        {
        case 0:
            attrMap["品牌"].docIdMap_["Two In One/欧艾尼"].push_back(it->docId_);
            ++attrMap["品牌"].docCount_;
            attrMap["领子"].docIdMap_["圆领"].push_back(it->docId_);
            ++attrMap["领子"].docCount_;
            break;
        case 1:
            attrMap["品牌"].docIdMap_["阿依莲"].push_back(it->docId_);
            ++attrMap["品牌"].docCount_;
            attrMap["季节"].docIdMap_["春季"].push_back(it->docId_);
            attrMap["季节"].docIdMap_["夏季"].push_back(it->docId_);
            attrMap["季节"].docIdMap_["秋季"].push_back(it->docId_);
            ++attrMap["季节"].docCount_;
            attrMap["价格"].docIdMap_["2011"].push_back(it->docId_);
            ++attrMap["价格"].docCount_;
            break;
        case 2:
            break;
        case 3:
            attrMap["品牌"].docIdMap_["淑女屋"].push_back(it->docId_);
            ++attrMap["品牌"].docCount_;
            attrMap["季节"].docIdMap_["夏季"].push_back(it->docId_);
            ++attrMap["季节"].docCount_;
            attrMap["年份"].docIdMap_["2011"].push_back(it->docId_);
            ++attrMap["年份"].docCount_;
            attrMap["尺码"].docIdMap_["S"].push_back(it->docId_);
            attrMap["尺码"].docIdMap_["M"].push_back(it->docId_);
            attrMap["尺码"].docIdMap_["L"].push_back(it->docId_);
            attrMap["尺码"].docIdMap_["XL"].push_back(it->docId_);
            ++attrMap["尺码"].docCount_;
            break;
        default:
            BOOST_ASSERT(false);
        }
    }
}

void checkGroupRep(
    const faceted::OntologyRep& groupRep,
    AttrMap& attrMap
)
{
    typedef list<faceted::OntologyRepItem> RepItemList;
    const RepItemList& itemList = groupRep.item_list;

    // check list size
    unsigned int totalCount = 0;
    // iterate property name
    for (AttrMap::const_iterator it = attrMap.begin();
        it != attrMap.end(); ++it)
    {
        ++totalCount;
        // get property value count
        totalCount += it->second.docIdMap_.size();
    }
    BOOST_CHECK_EQUAL(itemList.size(), totalCount);

    string attrName, convertBuffer;
    for (RepItemList::const_iterator it = itemList.begin();
            it != itemList.end(); ++it)
    {
        const faceted::OntologyRepItem& item = *it;
        item.text.convertString(convertBuffer, ENCODING_TYPE);

        if (item.level == 0)
        {
            attrName = convertBuffer;
            BOOST_TEST_MESSAGE("check attribute name: " << attrName
                               << ", doc count: " << item.doc_count);
            BOOST_CHECK_EQUAL(item.doc_count, attrMap[attrName].docCount_);
            BOOST_CHECK_EQUAL(item.doc_id_list.size(), 0);
        }
        else
        {
            BOOST_CHECK_EQUAL(item.level, 1);

            DocIdList& docIdList = attrMap[attrName].docIdMap_[convertBuffer];
            BOOST_TEST_MESSAGE("check attribute name: " << attrName
                               << ", value: " << convertBuffer
                               << ", doc count: " << item.doc_count);
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
    pconfig3.setName(PROP_NAME_ATTR);
    pconfig3.setType(sf1r::STRING_PROPERTY_TYPE);

    propertyConfig.insert(pconfig1);
    propertyConfig.insert(pconfig2);
    propertyConfig.insert(pconfig3);
}

DocumentManager*
createDocumentManager()
{
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

    property.assign(docInput.attrStr_, ENCODING_TYPE);
    document.property(PROP_NAME_ATTR) = property;
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
    value.convertString(utf8Str, ENCODING_TYPE);
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
    checkProperty(doc, PROP_NAME_ATTR, docInput.attrStr_);
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

void checkAttrManager(
    DocumentManager* documentManager,
    const vector<DocInput>& docInputVec,
    bool isProcessCollection
)
{
    bfs::path attrPath(bfs::path(TEST_DIR_STR) / "attr");
    faceted::AttrManager* attrManager = new faceted::AttrManager(documentManager, attrPath.string());

    AttrConfig attrConfig;
    attrConfig.propName = PROP_NAME_ATTR;

    BOOST_CHECK(attrManager->open(attrConfig));

    if (isProcessCollection)
    {
        BOOST_CHECK(attrManager->processCollection());
    }

    vector<unsigned int> docIdList;
    for (vector<DocInput>::const_iterator it = docInputVec.begin();
        it != docInputVec.end(); ++it)
    {
        docIdList.push_back(it->docId_);
    }

    std::vector<std::pair<std::string, std::string> > labelList;
    faceted::OntologyRep groupRep;
    BOOST_CHECK(attrManager->getGroupRep(docIdList, labelList, NULL, groupRep));

    AttrMap attrMap;
    createAttrMap(docInputVec, attrMap);
    checkGroupRep(groupRep, attrMap);

    delete attrManager;
}

typedef pair<string, vector<string> > AttrPairStr;
void checkAttrPairs(
    const vector<faceted::AttrManager::AttrPair>& attrPairs,
    const vector<AttrPairStr>& goldAttrPairs
)
{
    BOOST_CHECK_EQUAL(attrPairs.size(), goldAttrPairs.size());
    for (unsigned int i = 0; i < attrPairs.size(); ++i)
    {
        std::string nameUtf8;
        attrPairs[i].first.convertString(nameUtf8, ENCODING_TYPE);
        BOOST_CHECK_EQUAL(nameUtf8, goldAttrPairs[i].first);

        BOOST_CHECK_EQUAL(attrPairs[i].second.size(), goldAttrPairs[i].second.size());
        for (unsigned int j = 0; j < attrPairs[i].second.size(); ++j)
        {
            std::string valueUtf8;
            attrPairs[i].second[j].convertString(valueUtf8, ENCODING_TYPE);
            BOOST_CHECK_EQUAL(valueUtf8, goldAttrPairs[i].second[j]);
        }
    }
}

}

BOOST_AUTO_TEST_SUITE(AttrManager_test)

BOOST_AUTO_TEST_CASE(splitAttrPair)
{
    {
        izenelib::util::UString src(",,:,", ENCODING_TYPE);
        vector<faceted::AttrManager::AttrPair> attrPairs;
        faceted::AttrManager::splitAttrPair(src, attrPairs);
        BOOST_CHECK_EQUAL(attrPairs.size(), 0);
    }

    {
        izenelib::util::UString src("abcde:", ENCODING_TYPE);
        vector<faceted::AttrManager::AttrPair> attrPairs;
        faceted::AttrManager::splitAttrPair(src, attrPairs);
        BOOST_CHECK_EQUAL(attrPairs.size(), 0);
    }

    {
        izenelib::util::UString src("品牌: Two In One/欧艾尼 ,英文名:\"John, \nMark: \"\"Mary\"\" | Tom\",领子:圆领,季节:春季|\"夏|季\" abc |秋季,年份:2011,尺码:S|\" M \"| L|XL  ,,,:,,", ENCODING_TYPE);
        vector<faceted::AttrManager::AttrPair> attrPairs;
        faceted::AttrManager::splitAttrPair(src, attrPairs);

        vector<AttrPairStr> goldAttrPairs;
        {
            AttrPairStr pairStr;
            pairStr.first = "品牌";
            pairStr.second.push_back("Two In One/欧艾尼");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "英文名";
            pairStr.second.push_back("John, \nMark: \"Mary\" | Tom");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "领子";
            pairStr.second.push_back("圆领");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "季节";
            pairStr.second.push_back("春季");
            pairStr.second.push_back("夏|季");
            pairStr.second.push_back("秋季");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "年份";
            pairStr.second.push_back("2011");
            goldAttrPairs.push_back(pairStr);
        }
        {
            AttrPairStr pairStr;
            pairStr.first = "尺码";
            pairStr.second.push_back("S");
            pairStr.second.push_back(" M ");
            pairStr.second.push_back("L");
            pairStr.second.push_back("XL");
            goldAttrPairs.push_back(pairStr);
        }

        checkAttrPairs(attrPairs, goldAttrPairs);
    }
}

BOOST_AUTO_TEST_CASE(getGroupRep)
{
    boost::filesystem::remove_all(TEST_DIR_STR);

    DocumentManager* documentManager = createDocumentManager();
    BOOST_REQUIRE(documentManager != NULL);

    vector<DocInput> docInputVec;

    BOOST_TEST_MESSAGE("check empty attr index");
    checkAttrManager(documentManager, docInputVec, false);

    createDocInput(docInputVec, 1, 100);
    createCollection(documentManager, docInputVec);
    checkCollection(documentManager, docInputVec);

    BOOST_TEST_MESSAGE("create attr index 1st time");
    checkAttrManager(documentManager, docInputVec, true);

    BOOST_TEST_MESSAGE("load attr index");
    checkAttrManager(documentManager, docInputVec, false);

    vector<DocInput> newDocInputVec;
    createDocInput(newDocInputVec, 101, 200);
    createCollection(documentManager, newDocInputVec);
    checkCollection(documentManager, newDocInputVec);

    BOOST_TEST_MESSAGE("create attr index 2nd time");
    docInputVec.insert(docInputVec.end(), newDocInputVec.begin(), newDocInputVec.end());
    checkAttrManager(documentManager, docInputVec, true);

    BOOST_TEST_MESSAGE("load attr index");
    checkAttrManager(documentManager, docInputVec, false);

    delete documentManager;
}

BOOST_AUTO_TEST_SUITE_END() 

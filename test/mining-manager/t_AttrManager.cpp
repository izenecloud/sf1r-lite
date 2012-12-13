///
/// @file t_AttrManager.cpp
/// @brief test attrby functions
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-23
///
#include <mining-manager/MiningTask.h>
#include <mining-manager/MiningTaskBuilder.h>
#include <util/ustring/UString.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <mining-manager/attr-manager/AttrManager.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/group-manager/PropSharedLockSet.h>
#include <mining-manager/group-manager/GroupRep.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/GroupConfig.h>

#include <cstdlib>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <map>
#include <list>
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
}

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
        title_ = "Title ";
        title_ += lexical_cast<string>(docId);
    }
};

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

typedef vector<unsigned int> DocIdList;
struct ValueMap
{
    ValueMap(): docCount_(0) {}

    map<string, DocIdList> docIdMap_; // key: attribute value
    unsigned int docCount_;
};
typedef map<string, ValueMap> AttrMap; // key: attribute name

class AttrManagerTestFixture
{
private:
    set<PropertyConfig, PropertyComp> schema_;

    boost::shared_ptr<DocumentManager> documentManager_;
    vector<DocInput> docInputVec_;
    vector<unsigned int> docIdList_;

    string attrPath_;
    AttrConfig attrConfig_;
    faceted::AttrManager* attrManager_;
    MiningTaskBuilder* miningTaskBuilder_;

public:
    AttrManagerTestFixture()
        : attrManager_(NULL)
        , miningTaskBuilder_(NULL)
    {
        boost::filesystem::remove_all(TEST_DIR_STR);
        bfs::path dmPath(bfs::path(TEST_DIR_STR) / "dm/");
        bfs::create_directories(dmPath);
        attrPath_ = (bfs::path(TEST_DIR_STR) / "attr").string();

        initConfig_();
     
        boost::shared_ptr<DocumentManager> ret(new DocumentManager(
            dmPath.string(),
            schema_,
            ENCODING_TYPE,
            2000));
        documentManager_ = ret;

        resetAttrManager();
    }

    ~AttrManagerTestFixture()
    {
        delete attrManager_;
        delete miningTaskBuilder_;
    }

    void resetAttrManager()
    {
        delete attrManager_;
        delete miningTaskBuilder_;

        attrManager_ = new faceted::AttrManager(attrConfig_, attrPath_, *documentManager_);
        BOOST_CHECK(attrManager_->open());

        miningTaskBuilder_ = new MiningTaskBuilder(documentManager_);
        BOOST_CHECK(miningTaskBuilder_);
        MiningTask* miningTask = attrManager_->getAttrMiningTask();
        miningTaskBuilder_->addTask(miningTask);
    }

    void createDocument(int start, int end)
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

            docInputVec_.push_back(docInput);
            docIdList_.push_back(i);

            Document document;
            prepareDocument(document, docInput);
            BOOST_CHECK(documentManager_->insertDocument(document));
        }

        checkCollection_();

        BOOST_CHECK(miningTaskBuilder_->buildCollection());
        //BOOST_CHECK(attrManager_->processCollection());
    }

    void checkAttrManager()
    {
        faceted::OntologyRep groupRep;
        createGroupRep_(groupRep);

        checkAttrRepMerge(groupRep);

        AttrMap attrMap;
        createAttrMap_(attrMap);

        checkGroupRep_(groupRep, attrMap);
    }

private:
    void initConfig_()
    {
        PropertyConfigBase config1;
        config1.propertyName_ = "DOCID";
        config1.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config1);

        PropertyConfigBase config2;
        config2.propertyName_ = "Title";
        config2.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config2);

        PropertyConfigBase config3;
        config3.propertyName_ = PROP_NAME_ATTR;
        config3.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config3);

        attrConfig_.propName = PROP_NAME_ATTR;
    }

    void checkCollection_()
    {
        for (vector<DocInput>::const_iterator it = docInputVec_.begin();
            it != docInputVec_.end(); ++it)
        {
            const DocInput& docInput = *it;
            Document doc;
            documentManager_->getDocument(docInput.docId_, doc);

            checkProperty(doc, "DOCID", lexical_cast<string>(docInput.docId_));
            checkProperty(doc, PROP_NAME_ATTR, docInput.attrStr_);
        }
    }

    void createGroupRep_(faceted::OntologyRep& attrRep)
    {
        const GroupConfigMap emptyGroupConfigMap;
        faceted::GroupFilterBuilder filterBuilder(emptyGroupConfigMap, NULL, attrManager_, NULL);
        faceted::GroupParam groupParam;
        groupParam.isAttrGroup_ = true;

        faceted::PropSharedLockSet propSharedLockSet;
        faceted::GroupFilter* filter =
            filterBuilder.createFilter(groupParam, propSharedLockSet);

        for (vector<unsigned int>::const_iterator it = docIdList_.begin();
            it != docIdList_.end(); ++it)
        {
            BOOST_CHECK(filter->test(*it));
        }

        faceted::GroupRep groupRep;
        filter->getGroupRep(groupRep, attrRep);

        delete filter;
    }

    void checkAttrRepMerge(const faceted::OntologyRep& attrRep)
    {
        using namespace faceted;
        if(attrRep.item_list.size() == 0)
            return;
        CategoryIdType valueid = 0;
        std::list<OntologyRepItem>::const_iterator orig_it = attrRep.item_list.begin();
        while(orig_it != attrRep.item_list.end())
        {
            if(orig_it->level == 0)
            {
                valueid = 0;
            }
            else
            {
                BOOST_CHECK(orig_it->id >= valueid);
                valueid = orig_it->id;
            }
            ++orig_it;
        }
        BOOST_TEST_MESSAGE("check attr merge: ");
        faceted::OntologyRep testattrRep;
        std::list<OntologyRep*> others_rep;
        std::vector<OntologyRep> others_rep_dup;
        std::vector<OntologyRep> others_rep_dup_tmp;
        std::vector<OntologyRep> empty_rep;
        empty_rep.resize(10);
        for(int i = 0; i < 10; ++i)
        {
            others_rep_dup.push_back(attrRep);
        }
        //test empty merge
        testattrRep.merge(0, others_rep);
        BOOST_CHECK_EQUAL(testattrRep.item_list.size(), 0);
        // test only 1 merge
        others_rep_dup_tmp = others_rep_dup;
        others_rep.push_back(&others_rep_dup_tmp[0]);
        testattrRep.merge(0, others_rep);
        BOOST_CHECK_EQUAL(testattrRep.item_list.size(), attrRep.item_list.size());
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            attrRep.item_list.begin()->doc_count);
        others_rep.clear();
        testattrRep = OntologyRep();

        others_rep_dup_tmp = others_rep_dup;
        others_rep.push_back(&others_rep_dup_tmp[0]);
        testattrRep.merge(1, others_rep);
        OntologyRep::item_iterator it = testattrRep.Begin();
        int topN = 0;
        size_t current_maxdoc = INT_MAX;
        while(it != testattrRep.End())
        {
            if(it->level == 0)
            {
                ++topN;
                BOOST_CHECK(it->doc_count <= current_maxdoc);
                current_maxdoc = it->doc_count;
            }
            ++it;
        }
        BOOST_CHECK(topN <= 1);
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            attrRep.item_list.begin()->doc_count);
        others_rep.clear();
        testattrRep = OntologyRep();

        // test several empty merge
        for(int i = 0; i < 5; ++i)
        {
            others_rep.push_back(&empty_rep[i]);
        }
        others_rep_dup_tmp = others_rep_dup;
        others_rep.push_back(&others_rep_dup_tmp[0]);
        testattrRep.merge(0, others_rep);
        BOOST_CHECK_EQUAL(testattrRep.item_list.size(), attrRep.item_list.size());
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            attrRep.item_list.begin()->doc_count);
        others_rep.clear();
        testattrRep = OntologyRep();

        // test common merge
        //
        others_rep_dup_tmp = others_rep_dup;
        for(int i = 0; i < 10; ++i)
        {
            others_rep.push_back(&others_rep_dup_tmp[i]);
            others_rep.push_back(&empty_rep[i]);
        }
        testattrRep.merge(0, others_rep);
        BOOST_CHECK_EQUAL(testattrRep.item_list.size(), attrRep.item_list.size());
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            10*attrRep.item_list.begin()->doc_count);
        others_rep.clear();
        others_rep_dup_tmp = others_rep_dup;
        for(int i = 0; i < 10; ++i)
        {
            others_rep.push_back(&others_rep_dup_tmp[i]);
            others_rep.push_back(&empty_rep[i]);
        }
        testattrRep.merge(0, others_rep);
        BOOST_CHECK_EQUAL(testattrRep.item_list.size(), attrRep.item_list.size());
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            20*attrRep.item_list.begin()->doc_count);
        others_rep.clear();
        testattrRep.merge(5, others_rep);
        topN = 0;
        current_maxdoc = INT_MAX;
        it = testattrRep.Begin();
        while(it != testattrRep.End())
        {
            if(it->level == 0)
            {
                ++topN;
                BOOST_CHECK(it->doc_count <= current_maxdoc);
                current_maxdoc = it->doc_count;
            }
            ++it;
        }
        BOOST_CHECK(topN <= 5);
        BOOST_CHECK_EQUAL(testattrRep.item_list.begin()->doc_count, 
            20*attrRep.item_list.begin()->doc_count);
        it = testattrRep.Begin();
        valueid = 0;
        while(it != testattrRep.End())
        {
            if(it->level == 0)
            {
                valueid = 0;
            }
            else
            {
                BOOST_CHECK(it->id >= valueid);
                valueid = it->id;
            }
            ++it;
        }

    }

    void createAttrMap_(AttrMap& attrMap)
    {
        for (vector<DocInput>::const_iterator it = docInputVec_.begin();
            it != docInputVec_.end(); ++it)
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

    void checkGroupRep_(
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
            }
            else
            {
                BOOST_CHECK_EQUAL(item.level, 1);

                DocIdList& docIdList = attrMap[attrName].docIdMap_[convertBuffer];
                BOOST_TEST_MESSAGE("check attribute name: " << attrName
                                << ", value: " << convertBuffer
                                << ", doc count: " << item.doc_count);
                BOOST_CHECK_EQUAL(item.doc_count, docIdList.size());
            }
        }
    }
};

BOOST_AUTO_TEST_SUITE(AttrManager_test)

BOOST_FIXTURE_TEST_CASE(checkGetGroupRep, AttrManagerTestFixture)
{
    BOOST_TEST_MESSAGE("check empty attr index");
    checkAttrManager();

    BOOST_TEST_MESSAGE("create attr index 1st time");
    createDocument(1, 100);
    checkAttrManager();

    BOOST_TEST_MESSAGE("load attr index");
    resetAttrManager();
    checkAttrManager();

    BOOST_TEST_MESSAGE("create attr index 2nd time");
    createDocument(101, 200);
    checkAttrManager();

    BOOST_TEST_MESSAGE("load attr index");
    resetAttrManager();
    checkAttrManager();
}

BOOST_AUTO_TEST_SUITE_END() 

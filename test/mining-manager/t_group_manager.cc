///
/// @file t_group_manager.cpp
/// @brief test getting doc list for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-23
///
//TODO add test cases for numeric property
#include <util/ustring/UString.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <mining-manager/faceted-submanager/group_manager.h>
#include <mining-manager/faceted-submanager/GroupParam.h>
#include <mining-manager/faceted-submanager/GroupFilterBuilder.h>
#include <mining-manager/faceted-submanager/GroupFilter.h>
#include <configuration-manager/PropertyConfig.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <cstdlib>

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
}

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
        const string& groupStr,
        const string& groupInt,
        const string& groupFloat
    )
    : docId_(docId)
    , groupStr_(groupStr)
    , groupInt_(groupInt)
    , groupFloat_(groupFloat)
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
    value.convertString(utf8Str, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL(utf8Str, propValue);
}

void prepareDocument(
    Document& document,
    const DocInput& docInput
)
{
    document.setId(docInput.docId_);

    izenelib::util::UString property;
    property.assign(lexical_cast<string>(docInput.docId_), ENCODING_TYPE);
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

class GroupManagerTestFixture
{
private:
    set<PropertyConfig, PropertyComp> schema_;
    vector<string> propNames_;
    vector<GroupConfig> groupConfigs_;

    DocumentManager* documentManager_;
    vector<DocInput> docInputVec_;
    vector<unsigned int> docIdList_;

    string groupPath_;
    faceted::GroupManager* groupManager_;

    typedef vector<unsigned int> DocIdList;
    typedef map<string, DocIdList> DocIdMap; // key: property value
    typedef map<string, DocIdMap> PropertyMap; // key: property name

public:
    GroupManagerTestFixture()
        : documentManager_(NULL)
        , groupManager_(NULL)
    {
        boost::filesystem::remove_all(TEST_DIR_STR);
        bfs::path dmPath(bfs::path(TEST_DIR_STR) / "dm/");
        bfs::create_directories(dmPath);
        groupPath_ = (bfs::path(TEST_DIR_STR) / "group").string();

        initConfig_();

        documentManager_ = new DocumentManager(
            dmPath.string(),
            schema_,
            ENCODING_TYPE,
            2000);

        resetGroupManager();
    }

    ~GroupManagerTestFixture()
    {
        delete documentManager_;
        delete groupManager_;
    }

    void resetGroupManager()
    {
        delete groupManager_;

        groupManager_ = new faceted::GroupManager(documentManager_, groupPath_);
        BOOST_CHECK(groupManager_->open(groupConfigs_));
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

            docInputVec_.push_back(docInput);
            docIdList_.push_back(i);

            Document document;
            prepareDocument(document, docInput);
            BOOST_CHECK(documentManager_->insertDocument(document));
        }

        checkCollection_();
    }

    void checkGroupManager(bool isProcessCollection)
    {
        if (isProcessCollection)
        {
            BOOST_CHECK(groupManager_->processCollection());
        }

        faceted::GroupParam::GroupLabelVec labelList;

        createAndCheckGroupRep_(labelList);

        {
            faceted::GroupParam::GroupLabel label1;
            label1.first = PROP_NAME_GROUP_STR;
            label1.second.push_back("aaa");

            faceted::GroupParam::GroupLabel label2;
            label2.first = PROP_NAME_GROUP_INT;
            label2.second.push_back("2");

            labelList.push_back(label1);
            labelList.push_back(label2);

            createAndCheckGroupRep_(labelList);
        }
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
        config3.propertyName_ = PROP_NAME_GROUP_STR;
        config3.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config3);
        propNames_.push_back(config3.propertyName_);
        groupConfigs_.push_back(GroupConfig(config3.propertyName_, config3.propertyType_));

        PropertyConfigBase config4;
        config4.propertyName_ = PROP_NAME_GROUP_INT;
        config4.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config4);
        propNames_.push_back(config4.propertyName_);
        groupConfigs_.push_back(GroupConfig(config4.propertyName_, config4.propertyType_));

        PropertyConfigBase config5;
        config5.propertyName_ = PROP_NAME_GROUP_FLOAT;
        config5.propertyType_ = STRING_PROPERTY_TYPE;
        schema_.insert(config5);
        propNames_.push_back(config5.propertyName_);
        groupConfigs_.push_back(GroupConfig(config5.propertyName_, config5.propertyType_));
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
            checkProperty(doc, PROP_NAME_GROUP_STR, docInput.groupStr_);
            checkProperty(doc, PROP_NAME_GROUP_INT, docInput.groupInt_);
            checkProperty(doc, PROP_NAME_GROUP_FLOAT, docInput.groupFloat_);
        }
    }

    void createAndCheckGroupRep_(const faceted::GroupParam::GroupLabelVec& labelList)
    {
        faceted::OntologyRep groupRep;
        PropertyMap propMap;

        BOOST_TEST_MESSAGE("check label size: " << labelList.size());
        createGroupRep_(labelList, groupRep);
        createPropertyMap_(labelList, propMap);
        checkGroupRep_(groupRep, propMap);
    }

    void createGroupRep_(
        const faceted::GroupParam::GroupLabelVec& labelVec,
        faceted::OntologyRep& groupRep
    )
    {
        faceted::GroupFilterBuilder filterBuilder(groupConfigs_, groupManager_, NULL, NULL);
        faceted::GroupParam groupParam;
        groupParam.groupProps_ = propNames_;
        groupParam.groupLabels_ = labelVec;

        faceted::GroupFilter* filter = filterBuilder.createFilter(groupParam);
        for (vector<unsigned int>::const_iterator it = docIdList_.begin();
            it != docIdList_.end(); ++it)
        {
            bool pass = filter->test(*it);
            if (labelVec.empty())
            {
                BOOST_CHECK(pass);
            }
        }

        faceted::OntologyRep attrRep;
        filter->getGroupRep(groupRep, attrRep);

        delete filter;
    }

    /**
    * @param labelList the label list, it is assumed that each path contains only one element.
    */
    void createPropertyMap_(
        const faceted::GroupParam::GroupLabelVec& labelList,
        PropertyMap& propertyMap
    )
    {
        DocIdMap& strDocIdMap = propertyMap[PROP_NAME_GROUP_STR];
        DocIdMap& intDocIdMap = propertyMap[PROP_NAME_GROUP_INT];
        DocIdMap& floatDocIdMap = propertyMap[PROP_NAME_GROUP_FLOAT];

        for (vector<DocInput>::const_iterator it = docInputVec_.begin();
            it != docInputVec_.end(); ++it)
        {
            bool isOK = true;
            for (std::size_t i = 0; i < labelList.size(); ++i)
            {
                const faceted::GroupParam::GroupLabel& label = labelList[i];
                if ((label.first == PROP_NAME_GROUP_INT && label.second[0] != it->groupInt_)
                    || (label.first == PROP_NAME_GROUP_FLOAT && label.second[0] != it->groupFloat_))
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
                const faceted::GroupParam::GroupLabel& label = labelList[i];
                if ((label.first == PROP_NAME_GROUP_STR && label.second[0] != it->groupStr_)
                    || (label.first == PROP_NAME_GROUP_FLOAT && label.second[0] != it->groupFloat_))
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
                const faceted::GroupParam::GroupLabel& label = labelList[i];
                if ((label.first == PROP_NAME_GROUP_STR && label.second[0] != it->groupStr_)
                    || (label.first == PROP_NAME_GROUP_INT && label.second[0] != it->groupInt_))
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

    void checkGroupRep_(
        const faceted::OntologyRep& groupRep,
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
        vector<string>::const_iterator propNameIt = propNames_.begin();
        int propDocCount = 0;
        int valueDocSum = 0;
        for (RepItemList::const_iterator it = itemList.begin();
                it != itemList.end(); ++it)
        {
            const faceted::OntologyRepItem& item = *it;
            item.text.convertString(convertBuffer, ENCODING_TYPE);

            if (item.level == 0)
            {
                propName = convertBuffer;
                BOOST_CHECK(propNameIt != propNames_.end());
                BOOST_CHECK_EQUAL(propName, *propNameIt);
                ++propNameIt;

                BOOST_CHECK_EQUAL(propDocCount, valueDocSum);
                propDocCount = item.doc_count;
                valueDocSum = 0;
            }
            else
            {
                BOOST_CHECK_EQUAL(item.level, 1);

                DocIdList& docIdList = propertyMap[propName][convertBuffer];
                BOOST_TEST_MESSAGE("check property: " << propName
                                << ", value: " << convertBuffer
                                << ", doc count: " << docIdList.size());
                BOOST_CHECK_EQUAL(item.doc_count, docIdList.size());

                valueDocSum += item.doc_count;
            }
        }
        BOOST_CHECK_EQUAL(propDocCount, valueDocSum);
    }
};

BOOST_AUTO_TEST_SUITE(GroupManager_test)

BOOST_FIXTURE_TEST_CASE(checkGetGroupRep, GroupManagerTestFixture)
{
    BOOST_TEST_MESSAGE("check empty group index");
    checkGroupManager(false);

    BOOST_TEST_MESSAGE("create group index 1st time");
    createDocument(1, 100);
    checkGroupManager(true);

    BOOST_TEST_MESSAGE("load group index");
    resetGroupManager();
    checkGroupManager(false);

    BOOST_TEST_MESSAGE("create group index 2nd time");
    createDocument(101, 200);
    checkGroupManager(true);
}

BOOST_AUTO_TEST_SUITE_END() 

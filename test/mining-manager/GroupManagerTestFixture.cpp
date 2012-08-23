#include "GroupManagerTestFixture.h"
#include "NumericPropertyTableBuilderStub.h"
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>
#include <mining-manager/group-manager/GroupManager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>

#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

using namespace std;
using namespace boost;

namespace bfs = boost::filesystem;

namespace
{
using namespace sf1r;

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const char* PROP_NAME_GROUP_STR = "Group_str";
const char* PROP_NAME_GROUP_INT = "Group_int";
const char* PROP_NAME_GROUP_FLOAT = "Group_float";
const char* PROP_NAME_GROUP_DATETIME = "Group_datetime";
const char* TEST_DIR_STR = "group_test";

string normalizeDoubleRep(double value)
{
    ostringstream oss;
    oss << fixed << setprecision(2);
    oss << value;
    return oss.str();
}

void checkProperty(
    const Document& doc,
    const string& propName,
    int propValue
)
{
    Document::property_const_iterator it = doc.findProperty(propName);
    BOOST_REQUIRE(it != doc.propertyEnd());

    const PropertyValue& value = it->second;
    BOOST_CHECK_EQUAL(value.get<int32_t>(), propValue);
}

void checkProperty(
    const Document& doc,
    const string& propName,
    float propValue
)
{
    Document::property_const_iterator it = doc.findProperty(propName);
    BOOST_REQUIRE(it != doc.propertyEnd());

    const PropertyValue& value = it->second;
    BOOST_CHECK_EQUAL(value.get<float>(), propValue);
}

void checkProperty(
    const Document& doc,
    const string& propName,
    const string& propValue
)
{
    Document::property_const_iterator it = doc.findProperty(propName);
    BOOST_REQUIRE(it != doc.propertyEnd());

    const PropertyValue& value = it->second;
    const izenelib::util::UString& ustr = value.get<izenelib::util::UString>();
    std::string utf8Str;
    ustr.convertString(utf8Str, ENCODING_TYPE);
    BOOST_CHECK_EQUAL(utf8Str, propValue);
}

void prepareDocument(
    Document& document,
    const GroupManagerTestFixture::DocInput& docInput
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

    property.assign(docInput.dateStr_, ENCODING_TYPE);
    document.property(PROP_NAME_GROUP_DATETIME) = property;

    document.property(PROP_NAME_GROUP_INT) = lexical_cast<int32_t>(docInput.groupInt_);
    document.property(PROP_NAME_GROUP_FLOAT) = lexical_cast<float>(docInput.groupFloat_);
}

void create_EmptyLabel(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();
}

void create_OneLabel_PropStr(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path;
    path.push_back("aaa");
    labels[PROP_NAME_GROUP_STR].push_back(path);
}

void create_OneLabel_PropDate(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path;
    path.push_back("2012-07");
    labels[PROP_NAME_GROUP_DATETIME].push_back(path);
}

void create_TwoLabel_OneProperty(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path1;
    path1.push_back("上海");
    labels[PROP_NAME_GROUP_STR].push_back(path1);

    faceted::GroupParam::GroupPath path2;
    path2.push_back("中国");
    labels[PROP_NAME_GROUP_STR].push_back(path2);
}

void create_TwoLabel_TwoProperty(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path1;
    path1.push_back("上海");
    labels[PROP_NAME_GROUP_STR].push_back(path1);

    faceted::GroupParam::GroupPath path2;
    path2.push_back("1");
    labels[PROP_NAME_GROUP_INT].push_back(path2);
}

void create_ThreeLabel_OneProperty(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path1;
    path1.push_back("aaa");
    labels[PROP_NAME_GROUP_STR].push_back(path1);

    faceted::GroupParam::GroupPath path2;
    path2.push_back("中国");
    labels[PROP_NAME_GROUP_STR].push_back(path2);

    faceted::GroupParam::GroupPath path3;
    path3.push_back("上海");
    labels[PROP_NAME_GROUP_STR].push_back(path3);
}

void create_ThreeLabel_TwoProperty(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path;
    path.push_back("aaa");
    labels[PROP_NAME_GROUP_STR].push_back(path);

    faceted::GroupParam::GroupPath path2;
    path2.push_back("0.2");
    labels[PROP_NAME_GROUP_FLOAT].push_back(path2);

    faceted::GroupParam::GroupPath path3;
    path3.push_back("0.3");
    labels[PROP_NAME_GROUP_FLOAT].push_back(path3);
}

void create_ThreeLabel_FourProperty(faceted::GroupParam::GroupLabelMap& labels)
{
    labels.clear();

    faceted::GroupParam::GroupPath path1;
    path1.push_back("aaa");
    labels[PROP_NAME_GROUP_STR].push_back(path1);

    faceted::GroupParam::GroupPath path2;
    path2.push_back("2");
    labels[PROP_NAME_GROUP_INT].push_back(path2);

    faceted::GroupParam::GroupPath path3;
    path3.push_back("0.3");
    labels[PROP_NAME_GROUP_FLOAT].push_back(path3);

    faceted::GroupParam::GroupPath path4;
    path4.push_back("2012-12");
    labels[PROP_NAME_GROUP_DATETIME].push_back(path4);
}

bool isDocBelongToStrLabel(
    const GroupManagerTestFixture::DocInput& docInput,
    const faceted::GroupParam::GroupLabelMap& labels
)
{
    faceted::GroupParam::GroupLabelMap::const_iterator findIt = labels.find(PROP_NAME_GROUP_STR);
    if (findIt == labels.end())
        return true;

    const faceted::GroupParam::GroupPathVec& paths = findIt->second;
    for (faceted::GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        if (pathIt->front() == docInput.groupStr_)
            return true;
    }

    return false;
}

bool isDocBelongToIntLabel(
    const GroupManagerTestFixture::DocInput& docInput,
    const faceted::GroupParam::GroupLabelMap& labels
)
{
    faceted::GroupParam::GroupLabelMap::const_iterator findIt = labels.find(PROP_NAME_GROUP_INT);
    if (findIt == labels.end())
        return true;

    const faceted::GroupParam::GroupPathVec& paths = findIt->second;
    for (faceted::GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        if (lexical_cast<int>(pathIt->front()) == docInput.groupInt_)
            return true;
    }

    return false;
}

bool isDocBelongToFloatLabel(
    const GroupManagerTestFixture::DocInput& docInput,
    const faceted::GroupParam::GroupLabelMap& labels
)
{
    faceted::GroupParam::GroupLabelMap::const_iterator findIt = labels.find(PROP_NAME_GROUP_FLOAT);
    if (findIt == labels.end())
        return true;

    const faceted::GroupParam::GroupPathVec& paths = findIt->second;
    for (faceted::GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        if (lexical_cast<float>(pathIt->front()) == docInput.groupFloat_)
            return true;
    }

    return false;
}

bool isDocBelongToDateLabel(
    const GroupManagerTestFixture::DocInput& docInput,
    const faceted::GroupParam::GroupLabelMap& labels
)
{
    faceted::GroupParam::GroupLabelMap::const_iterator findIt = labels.find(PROP_NAME_GROUP_DATETIME);
    if (findIt == labels.end())
        return true;

    const faceted::GroupParam::GroupPathVec& paths = findIt->second;
    for (faceted::GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        if (pathIt->front() == docInput.yearMonthStr_)
            return true;
    }

    return false;
}

}

namespace sf1r
{

GroupManagerTestFixture::DocInput::DocInput()
    : docId_(0)
    , groupInt_(0)
    , groupFloat_(0)
{
}

GroupManagerTestFixture::DocInput::DocInput(
    unsigned int docId,
    const std::string& groupStr,
    int groupInt,
    float groupFloat,
    const std::string& dateStr,
    const std::string& yearMonthStr
)
    : docId_(docId)
    , groupStr_(groupStr)
    , groupInt_(groupInt)
    , groupFloat_(groupFloat)
    , dateStr_(dateStr)
    , yearMonthStr_(yearMonthStr)
{
    title_ = "Title ";
    title_ += lexical_cast<std::string>(docId);
}

GroupManagerTestFixture::GroupManagerTestFixture()
    : documentManager_(NULL)
    , groupManager_(NULL)
    , numericTableBuilder_(NULL)
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

    numericTableBuilder_ = new NumericPropertyTableBuilderStub(groupConfigMap_);

    resetGroupManager();
}

GroupManagerTestFixture::~GroupManagerTestFixture()
{
    delete documentManager_;
    delete groupManager_;
    delete numericTableBuilder_;
}

void GroupManagerTestFixture::resetGroupManager()
{
    delete groupManager_;

    groupManager_ = new faceted::GroupManager(
        groupConfigMap_, *documentManager_, groupPath_);
    BOOST_CHECK(groupManager_->open());
}

void GroupManagerTestFixture::configGroupPropRebuild()
{
    for (GroupConfigMap::iterator it = groupConfigMap_.begin();
        it != groupConfigMap_.end(); ++it)
    {
        it->second.forceRebuild = true;
    }
}

void GroupManagerTestFixture::createDocument(int num)
{
    int lastDocId = 0;
    if (! docIdList_.empty())
    {
        lastDocId = docIdList_.back();
    }

    const int startDocId = lastDocId + 1;
    const int endDocId = lastDocId + num;
    for (int i = startDocId; i <= endDocId; ++i)
    {
        DocInput docInput;
        int mod = i % 4;
        switch (mod)
        {
        case 0:
            docInput = DocInput(i, "aaa", 1, 0.1, "20120720093000", "2012-07");
            break;
        case 1:
            docInput = DocInput(i, "上海", 2, 0.2, "20121008190000", "2012-10");
            break;
        case 2:
            docInput = DocInput(i, "中国", 3, 0.3, "20121211100908", "2012-12");
            break;
        case 3:
            docInput = DocInput(i, "aaa", 2, 0.3, "20121231235959", "2012-12");
            break;
        default:
            BOOST_ASSERT(false);
        }

        docInputVec_.push_back(docInput);
        docIdList_.push_back(i);

        Document document;
        prepareDocument(document, docInput);
        BOOST_CHECK(documentManager_->insertDocument(document));

        BOOST_CHECK(numericTableBuilder_->insertDocument(document));
    }

    checkCollection_();
    BOOST_CHECK(groupManager_->processCollection());
}

void GroupManagerTestFixture::checkGetGroupRep()
{
    faceted::GroupParam::GroupLabelMap labels;

    create_EmptyLabel(labels);
    createAndCheckGroupRep_(labels);

    create_OneLabel_PropStr(labels);
    createAndCheckGroupRep_(labels);

    create_OneLabel_PropDate(labels);
    createAndCheckGroupRep_(labels);

    create_TwoLabel_OneProperty(labels);
    createAndCheckGroupRep_(labels);

    create_TwoLabel_TwoProperty(labels);
    createAndCheckGroupRep_(labels);

    create_ThreeLabel_OneProperty(labels);
    createAndCheckGroupRep_(labels);

    create_ThreeLabel_TwoProperty(labels);
    createAndCheckGroupRep_(labels);

    create_ThreeLabel_FourProperty(labels);
    createAndCheckGroupRep_(labels);
}

void GroupManagerTestFixture::checkGroupRepMerge()
{
    faceted::GroupRep group, group0, group1, group2;

    izenelib::util::UString ua("a", ENCODING_TYPE);
    izenelib::util::UString ub("b", ENCODING_TYPE);
    izenelib::util::UString uc("c", ENCODING_TYPE);
    izenelib::util::UString ud("d", ENCODING_TYPE);
    izenelib::util::UString ue("e", ENCODING_TYPE);
    izenelib::util::UString uf("f", ENCODING_TYPE);
    izenelib::util::UString ug("g", ENCODING_TYPE);
    izenelib::util::UString uh("h", ENCODING_TYPE);
    izenelib::util::UString ui("i", ENCODING_TYPE);
    izenelib::util::UString uj("j", ENCODING_TYPE);

    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(0, ua, 0, 4));
    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ub, 0, 1));
    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ud, 0, 2));
    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(2, ug, 0, 1));
    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(2, uh, 0, 1));
    group1.stringGroupRep_.push_back(faceted::OntologyRepItem(1, uf, 0, 1));

    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(0, ua, 0, 5));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(1, uc, 0, 1));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ud, 0, 3));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(2, uh, 0, 1));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(2, ui, 0, 1));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(2, uj, 0, 1));
    group2.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ue, 0, 1));

    group.stringGroupRep_.push_back(faceted::OntologyRepItem(0, ua, 0, 9));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ub, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(1, uc, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ud, 0, 5));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(2, ug, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(2, uh, 0, 2));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(2, ui, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(2, uj, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(1, ue, 0, 1));
    group.stringGroupRep_.push_back(faceted::OntologyRepItem(1, uf, 0, 1));

    group0 = group1;
    group0.merge(group2);
    BOOST_CHECK_EQUAL( group == group0, true );

    group0 = group2;
    group0.merge(group1);
    BOOST_CHECK_EQUAL( group == group0, true );
}

void GroupManagerTestFixture::initConfig_()
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
    groupConfigMap_[config3.propertyName_] = GroupConfig(config3.propertyType_);

    PropertyConfigBase config4;
    config4.propertyName_ = PROP_NAME_GROUP_DATETIME;
    config4.propertyType_ = DATETIME_PROPERTY_TYPE;
    schema_.insert(config4);
    propNames_.push_back(config4.propertyName_);
    groupConfigMap_[config4.propertyName_] = GroupConfig(config4.propertyType_);

    PropertyConfigBase config5;
    config5.propertyName_ = PROP_NAME_GROUP_INT;
    config5.propertyType_ = INT32_PROPERTY_TYPE;
    schema_.insert(config5);
    propNames_.push_back(config5.propertyName_);
    groupConfigMap_[config5.propertyName_] = GroupConfig(config5.propertyType_);

    PropertyConfigBase config6;
    config6.propertyName_ = PROP_NAME_GROUP_FLOAT;
    config6.propertyType_ = FLOAT_PROPERTY_TYPE;
    schema_.insert(config6);
    propNames_.push_back(config6.propertyName_);
    groupConfigMap_[config6.propertyName_] = GroupConfig(config6.propertyType_);
}

void GroupManagerTestFixture::checkCollection_()
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
        checkProperty(doc, PROP_NAME_GROUP_DATETIME, docInput.dateStr_);
    }
}

void GroupManagerTestFixture::createAndCheckGroupRep_(const faceted::GroupParam::GroupLabelMap& labels)
{
    int labelNum = 0;
    for (faceted::GroupParam::GroupLabelMap::const_iterator it = labels.begin();
        it != labels.end(); ++it)
    {
        labelNum += it->second.size();
    }
    BOOST_TEST_MESSAGE("check label num: " << labelNum
                       << ", prop num: " << labels.size());

    faceted::GroupRep groupRep;
    PropertyMap propMap;

    createGroupRep_(labels, groupRep);
    createPropertyMap_(labels, propMap);
    checkGroupRep_(groupRep, propMap);
}

void GroupManagerTestFixture::createGroupRep_(
    const faceted::GroupParam::GroupLabelMap& labels,
    faceted::GroupRep& groupRep
)
{
    faceted::GroupFilterBuilder filterBuilder(
        groupConfigMap_, groupManager_, NULL, numericTableBuilder_);
    faceted::GroupParam groupParam;
    for (vector<string>::const_iterator it = propNames_.begin();
        it != propNames_.end(); ++it)
    {
        faceted::GroupPropParam propParam;
        propParam.property_ = *it;

        if (propParam.property_ == PROP_NAME_GROUP_DATETIME)
        {
            // groupby year-month
            propParam.unit_ = "M";
        }

        groupParam.groupProps_.push_back(propParam);
    }
    groupParam.groupLabels_ = labels;

    faceted::GroupFilter* filter = filterBuilder.createFilter(groupParam);
    for (vector<unsigned int>::const_iterator it = docIdList_.begin();
        it != docIdList_.end(); ++it)
    {
        bool pass = filter->test(*it);
        if (labels.empty())
        {
            BOOST_CHECK(pass);
        }
    }

    faceted::OntologyRep attrRep;
    filter->getGroupRep(groupRep, attrRep);
    groupRep.toOntologyRepItemList();

    delete filter;
}

void GroupManagerTestFixture::createPropertyMap_(
    const faceted::GroupParam::GroupLabelMap& labels,
    PropertyMap& propertyMap
) const
{
    DocIdMap& strDocIdMap = propertyMap[PROP_NAME_GROUP_STR];
    DocIdMap& intDocIdMap = propertyMap[PROP_NAME_GROUP_INT];
    DocIdMap& floatDocIdMap = propertyMap[PROP_NAME_GROUP_FLOAT];
    DocIdMap& dateDocIdMap = propertyMap[PROP_NAME_GROUP_DATETIME];

    for (vector<DocInput>::const_iterator it = docInputVec_.begin();
        it != docInputVec_.end(); ++it)
    {
        if (isDocBelongToIntLabel(*it, labels) &&
            isDocBelongToFloatLabel(*it, labels) &&
            isDocBelongToDateLabel(*it, labels))
        {
            strDocIdMap[it->groupStr_].push_back(it->docId_);
        }

        if (isDocBelongToStrLabel(*it, labels) &&
            isDocBelongToFloatLabel(*it, labels) &&
            isDocBelongToDateLabel(*it, labels))
        {
            string normRep = normalizeDoubleRep(it->groupInt_);
            intDocIdMap[normRep].push_back(it->docId_);
        }

        if (isDocBelongToStrLabel(*it, labels) &&
            isDocBelongToIntLabel(*it, labels) &&
            isDocBelongToDateLabel(*it, labels))
        {
            string normRep = normalizeDoubleRep(it->groupFloat_);
            floatDocIdMap[normRep].push_back(it->docId_);
        }

        if (isDocBelongToStrLabel(*it, labels) &&
            isDocBelongToIntLabel(*it, labels) &&
            isDocBelongToFloatLabel(*it, labels))
        {
            dateDocIdMap[it->yearMonthStr_].push_back(it->docId_);
        }
    }
}

void GroupManagerTestFixture::checkGroupRep_(
    const faceted::GroupRep& groupRep,
    PropertyMap& propertyMap
) const
{
    typedef list<faceted::OntologyRepItem> RepItemList;
    const RepItemList& itemList = groupRep.stringGroupRep_;

    // check list size
    unsigned int totalCount = 0;
    // iterate property name
    for (PropertyMap::const_iterator propIt = propertyMap.begin();
        propIt != propertyMap.end(); ++propIt)
    {
        ++totalCount;
        // get property value count
        totalCount += propIt->second.size();

        BOOST_TEST_MESSAGE("gold prop: " << propIt->first);
        const DocIdMap& docIdMap = propIt->second;
        for (DocIdMap::const_iterator valueIt = docIdMap.begin();
            valueIt != docIdMap.end(); ++valueIt)
        {
            BOOST_TEST_MESSAGE("  " << valueIt->first << ": " << valueIt->second.size());
        }
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

}

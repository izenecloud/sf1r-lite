#include "ItemManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/item/ItemManager.h>
#include <recommend-manager/item/ItemContainer.h>
#include <recommend-manager/item/LocalItemManager.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>

#include <cstdlib> // rand()
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

const char* PROP_NAME_DOCID = "DOCID";
const char* PROP_NAME_TITLE = "title";
const char* PROP_NAME_URL = "url";
const char* PROP_NAME_PRICE = "price";
const char* PROP_NAME_COUNT = "count";

const char* PROP_VALUE_DOCID = "DOCID_";
const char* PROP_VALUE_TITLE = "商品_";
const char* PROP_VALUE_URL = "http://www.e-commerce.com/item_";
}

namespace sf1r
{

ItemManagerTestFixture::ItemManagerTestFixture()
    : maxItemId_(0)
{
    initDMSchema_();
}

ItemManagerTestFixture::~ItemManagerTestFixture()
{
}

void ItemManagerTestFixture::setTestDir(const std::string& dir)
{
    testDir_ = dir;
    create_empty_directory(testDir_);
}

void ItemManagerTestFixture::resetInstance()
{
    BOOST_TEST_MESSAGE("create ItemManager");

    createDocManager_();

    // flush first
    itemManager_.reset();
    itemManager_.reset(new LocalItemManager(*documentManager_.get()));
}

void ItemManagerTestFixture::createDocManager_()
{
    // flush first
    documentManager_.reset();

    bfs::path dmDir(testDir_);
    dmDir /= "dm/";
    bfs::create_directory(dmDir);

    documentManager_.reset(new DocumentManager(dmDir.string(), indexSchema_, ENCODING_TYPE, 2000));
}

void ItemManagerTestFixture::checkItemManager()
{
    BOOST_TEST_MESSAGE("check " << itemMap_.size() << " items");

    Document doc;
    SingleItemContainer itemContainer(doc);
    for (itemid_t i = 1; i <= maxItemId_; ++i)
    {
        doc.setId(i);

        ItemMap::const_iterator findIt = itemMap_.find(i);
        if (findIt != itemMap_.end())
        {
            BOOST_CHECK(itemManager_->getItemProps(propList_, itemContainer));
            checkItem_(doc, findIt->second);
            BOOST_CHECK_MESSAGE(itemManager_->hasItem(i), "item id: " << i);
        }
        else
        {
            BOOST_CHECK(itemManager_->getItemProps(propList_, itemContainer) == false);
            BOOST_CHECK(doc.isEmpty());
            BOOST_CHECK(itemManager_->hasItem(i) == false);
        }
    }

    itemid_t nonExistId = maxItemId_ + 1;
    doc.setId(nonExistId);
    BOOST_CHECK(itemManager_->getItemProps(propList_, itemContainer) == false);
    BOOST_CHECK(doc.isEmpty());
    BOOST_CHECK(itemManager_->hasItem(nonExistId) == false);
}

void ItemManagerTestFixture::createItems(int num)
{
    izenelib::util::UString ustr;

    for (int i=0; i<num; ++i)
    {
        itemid_t id = ++maxItemId_;
        Document doc;
        ItemInput itemInput;

        prepareDoc_(id, doc, itemInput);

        BOOST_CHECK(documentManager_->insertDocument(doc));
        itemMap_[id] = itemInput;
    }

    BOOST_TEST_MESSAGE("create " << num << " items...");
}

void ItemManagerTestFixture::updateItems()
{
    int count = 0;
    for (ItemMap::iterator it = itemMap_.begin();
        it != itemMap_.end();)
    {
        if (rand() % 3)
        {
            ++it;
            continue;
        }

        Document doc;
        ItemInput itemInput;
        itemid_t id = it->first;
        prepareDoc_(id, doc, itemInput);

        BOOST_CHECK(documentManager_->updateDocument(doc));
        it->second = itemInput;

        ++count;
    }

    BOOST_TEST_MESSAGE("update " << count << " items...");
}

void ItemManagerTestFixture::removeItems()
{
    int count = 0;
    for (ItemMap::iterator it = itemMap_.begin();
        it != itemMap_.end();)
    {
        if (rand() % 2)
        {
            ++it;
            continue;
        }

        itemid_t id = it->first;
        BOOST_CHECK(documentManager_->removeDocument(id));
        itemMap_.erase(it++);

        ++count;
    }

    BOOST_TEST_MESSAGE("remove " << count << " items...");
}

void ItemManagerTestFixture::initDMSchema_()
{
    PropertyConfigBase config1;
    config1.propertyName_ = PROP_NAME_DOCID;
    config1.propertyType_ = STRING_PROPERTY_TYPE;
    indexSchema_.insert(config1);
    propList_.push_back(config1.propertyName_);

    PropertyConfigBase config2;
    config2.propertyName_ = PROP_NAME_TITLE;
    config2.propertyType_ = STRING_PROPERTY_TYPE;
    indexSchema_.insert(config2);
    propList_.push_back(config2.propertyName_);

    PropertyConfigBase config3;
    config3.propertyName_ = PROP_NAME_URL;
    config3.propertyType_ = STRING_PROPERTY_TYPE;
    indexSchema_.insert(config3);
    propList_.push_back(config3.propertyName_);

    PropertyConfigBase config4;
    config4.propertyName_ = PROP_NAME_PRICE;
    config4.propertyType_ = FLOAT_PROPERTY_TYPE;
    indexSchema_.insert(config4);
    propList_.push_back(config4.propertyName_);

    PropertyConfigBase config5;
    config5.propertyName_ = PROP_NAME_COUNT;
    config5.propertyType_ = INT32_PROPERTY_TYPE;
    indexSchema_.insert(config5);
    propList_.push_back(config5.propertyName_);
}

void ItemManagerTestFixture::checkItem_(const Document& doc, const ItemInput& itemInput)
{
    for (ItemInput::const_iterator it = itemInput.begin();
        it != itemInput.end(); ++it)
    {
        const string& propName = it->first;
        BOOST_REQUIRE_MESSAGE(doc.hasProperty(propName), propName);

        const PropertyValue propValue = doc.property(propName);
        const izenelib::util::UString& ustr = propValue.get<izenelib::util::UString>();
        BOOST_CHECK_EQUAL(ustr, it->second);
    }
}

void ItemManagerTestFixture::prepareDoc_(
    itemid_t id,
    Document& doc,
    ItemInput& itemInput)
{
    doc.setId(id);

    const string idStr = boost::lexical_cast<string>(id);
    insertItemId_(idStr, id);

    izenelib::util::UString ustr;

    ustr.assign(PROP_VALUE_DOCID + idStr, ENCODING_TYPE);
    itemInput[PROP_NAME_DOCID] = ustr;
    doc.property(PROP_NAME_DOCID) = ustr;

    int randValue = rand();
    const string randIdStr = boost::lexical_cast<string>(randValue);

    ustr.assign(PROP_VALUE_TITLE + randIdStr, ENCODING_TYPE);
    itemInput[PROP_NAME_TITLE] = ustr;
    doc.property(PROP_NAME_TITLE) = ustr;

    ustr.assign(PROP_VALUE_URL + randIdStr, ENCODING_TYPE);
    itemInput[PROP_NAME_URL] = ustr;
    doc.property(PROP_NAME_URL) = ustr;

    float f = static_cast<float>(randValue) / 3;
    ustr.assign(boost::lexical_cast<string>(f), ENCODING_TYPE);
    itemInput[PROP_NAME_PRICE] = ustr;
    doc.property(PROP_NAME_PRICE) = ustr;

    int c = randValue * 10;
    ustr.assign(boost::lexical_cast<string>(c), ENCODING_TYPE);
    itemInput[PROP_NAME_COUNT] = ustr;
    doc.property(PROP_NAME_COUNT) = ustr;
}

} // namespace sf1r

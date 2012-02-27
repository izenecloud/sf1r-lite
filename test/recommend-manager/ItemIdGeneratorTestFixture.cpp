#include "ItemIdGeneratorTestFixture.h"
#include <recommend-manager/item/ItemIdGenerator.h>
#include <util/test/BoostTestThreadSafety.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

namespace sf1r
{

ItemIdGeneratorTestFixture::ItemIdGeneratorTestFixture()
    : maxItemId_(0)
{
}

ItemIdGeneratorTestFixture::~ItemIdGeneratorTestFixture()
{
}

void ItemIdGeneratorTestFixture::checkNonExistId()
{
    BOOST_CHECK_EQUAL(itemIdGenerator_->maxItemId(), maxItemId_);

    std::string strId;
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(maxItemId_+1, strId) == false);
}

void ItemIdGeneratorTestFixture::checkStrToItemId(itemid_t maxItemId)
{
    for (itemid_t goldId=1; goldId<=maxItemId; ++goldId)
    {
        std::string str = boost::lexical_cast<std::string>(goldId);
        itemid_t id = 0;

        BOOST_CHECK_TS(itemIdGenerator_->strIdToItemId(str, id));
        BOOST_CHECK_EQUAL_TS(id, goldId);
    }

    BOOST_CHECK_EQUAL_TS(itemIdGenerator_->maxItemId(), maxItemId);

    maxItemId_ = maxItemId;
}

void ItemIdGeneratorTestFixture::checkItemIdToStr(itemid_t maxItemId)
{
    for (itemid_t id=1; id<=maxItemId; ++id)
    {
        std::string goldStr = boost::lexical_cast<std::string>(id);
        std::string str;

        BOOST_CHECK_TS(itemIdGenerator_->itemIdToStrId(id, str));
        BOOST_CHECK_EQUAL_TS(str, goldStr);
    }

    BOOST_CHECK_EQUAL_TS(itemIdGenerator_->maxItemId(), maxItemId);
}

void ItemIdGeneratorTestFixture::checkMultiThread(int threadNum, itemid_t maxItemId)
{
    boost::thread_group threads;

    for (int i=0; i<threadNum; ++i)
    {
        threads.create_thread(boost::bind(&ItemIdGeneratorTestFixture::checkStrToItemId,
                                          this, maxItemId));
    }

    threads.join_all();

    checkStrToItemId(maxItemId);
    checkItemIdToStr(maxItemId);
}

} // namespace sf1r

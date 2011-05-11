#include "PurchaseManager.h"
#include "ItemManager.h"
#include "ItemIterator.h"
#include <common/JobScheduler.h>

#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <fstream>
#include <glog/logging.h>

namespace
{

class CFTask
{
public:
    CFTask(
        sf1r::ItemCFManager* itemCFManager,
        sf1r::userid_t userId,
        sf1r::itemid_t maxItemId,
        std::list<sf1r::itemid_t>& oldItems,
        std::list<sf1r::itemid_t>& newItems
    )
    : itemCFManager_(itemCFManager)
    , userId_(userId)
    , maxItemId_(maxItemId)
    {
        oldItems_.swap(oldItems);
        newItems_.swap(newItems);
    }

    CFTask(const CFTask& cfTask)
    : itemCFManager_(cfTask.itemCFManager_)
    , userId_(cfTask.userId_)
    , maxItemId_(cfTask.maxItemId_)
    {
        oldItems_.swap(cfTask.oldItems_);
        newItems_.swap(cfTask.newItems_);
    }

    void purchase()
    {
        sf1r::ItemIterator itemIterator(1, maxItemId_);
        itemCFManager_->incrementalBuild(userId_, oldItems_, newItems_, itemIterator);
    }

private:
    sf1r::ItemCFManager* itemCFManager_;
    sf1r::userid_t userId_;
    sf1r::itemid_t maxItemId_;
    mutable std::list<sf1r::itemid_t> oldItems_;
    mutable std::list<sf1r::itemid_t> newItems_;
};

class OrderTask
{
public:
    OrderTask(
        sf1r::OrderManager& orderManager,
        sf1r::orderid_t orderId,
        std::list<sf1r::itemid_t>& orderItems
    )
    : orderManager_(orderManager)
    , orderId_(orderId)
    {
        orderItems_.swap(orderItems);
    }

    OrderTask(const OrderTask& orderTask)
    : orderManager_(orderTask.orderManager_)
    , orderId_(orderTask.orderId_)
    {
        orderItems_.swap(orderTask.orderItems_);
    }

    void purchase()
    {
        orderManager_.addOrder(orderId_, orderItems_);
    }

private:
    sf1r::OrderManager& orderManager_;
    sf1r::orderid_t orderId_;
    mutable std::list<sf1r::itemid_t> orderItems_;
};

}

namespace sf1r
{

PurchaseManager::PurchaseManager(
    const std::string& path,
    JobScheduler* jobScheduler,
    ItemCFManager* itemCFManager,
    const ItemManager* itemManager
)
    : container_(path)
    , orderManagerPath_(boost::filesystem::path(boost::filesystem::path(path).parent_path()/"order").string())
    , orderManager_(orderManagerPath_, itemManager)
    , jobScheduler_(jobScheduler)
    , itemCFManager_(itemCFManager)
    , itemManager_(itemManager)
    , orderIdPath_(boost::filesystem::path(boost::filesystem::path(orderManagerPath_)/"orderId.txt").string())
    , orderId_(0)
{
    container_.open();
    std::ifstream ifs(orderIdPath_.c_str());
    if (ifs)
    {
        ifs >> orderId_;
    }

}

PurchaseManager::~PurchaseManager()
{
    flush();
}

void PurchaseManager::flush()
{
    try
    {
        container_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }

    std::ofstream ofs(orderIdPath_.c_str());
    if (ofs)
    {
        ofs << orderId_;
    }
    else
    {
        LOG(ERROR) << "failed to write file " << orderIdPath_;
    }
}

bool PurchaseManager::addPurchaseItem(
    userid_t userId,
    const OrderItemVec& orderItemVec,
    const std::string& orderIdStr
)
{
    ItemIdSet itemIdSet;
    container_.getValue(userId, itemIdSet);

    std::list<sf1r::itemid_t> oldItems(itemIdSet.begin(), itemIdSet.end());
    std::list<sf1r::itemid_t> newItems;
    std::list<sf1r::itemid_t> orderItems;

    for (OrderItemVec::const_iterator it = orderItemVec.begin();
        it != orderItemVec.end(); ++it)
    {
        std::pair<ItemIdSet::iterator, bool> res = itemIdSet.insert(it->itemId_);
        if (res.second)
        {
            newItems.push_back(it->itemId_);
        }

        orderItems.push_back(it->itemId_);
    }

    {
        OrderTask task(orderManager_, newOrderId(), orderItems);
        jobScheduler_->addTask(boost::bind(&OrderTask::purchase, task));
    }

    // not purchased yet
    if (! newItems.empty())
    {
        bool result = false;
        try
        {
            result = container_.update(userId, itemIdSet);
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in SDB::update(): " << e.what();
        }

        if (result)
        {
            CFTask task(itemCFManager_, userId, itemManager_->maxItemId(), oldItems, newItems);
            jobScheduler_->addTask(boost::bind(&CFTask::purchase, task));
        }

        return result;
    }

    // already purchased
    return true;
}

bool PurchaseManager::getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet)
{
    bool result = false;
    try
    {
        itemIdSet.clear();
        container_.getValue(userId, itemIdSet);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

unsigned int PurchaseManager::purchaseUserNum()
{
    return container_.numItems();
}

PurchaseManager::SDBIterator PurchaseManager::begin()
{
    return SDBIterator(container_);
}

PurchaseManager::SDBIterator PurchaseManager::end()
{
    return SDBIterator();
}

orderid_t PurchaseManager::newOrderId()
{
    boost::mutex::scoped_lock lock(orderIdMutex_);
    return ++orderId_;
}

} // namespace sf1r

#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/PurchaseManager.h>
#include <recommend-manager/OrderManager.h>
#include <common/ScdParser.h>
#include <directory-manager/Directory.h>
#include <directory-manager/DirectoryRotator.h>
#include <common/JobScheduler.h>

#include <map>
#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace
{
/** the default encoding type */
const izenelib::util::UString::EncodingType DEFAULT_ENCODING = izenelib::util::UString::UTF_8;

/** the directory for scd file backup */
const char* SCD_BACKUP_DIR = "backup";

/** the max number of orders to collect before adding them into @c purchaseManager_ */
const unsigned int MAX_ORDER_NUM = 1000;

bool scanSCDFiles(const std::string& scdDir, std::vector<string>& scdList)
{
    // check the directory
    try
    {
        if (bfs::is_directory(scdDir) == false)
        {
            LOG(ERROR) << "path " << scdDir << " is not a directory";
            return false;
        }
    }
    catch(bfs::filesystem_error& e)
    {
        LOG(ERROR) << e.what();
        return false;
    }

    // search the directory for files
    const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdDir); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename();

            if (ScdParser::checkSCDFormat(fileName) )
            {
                scdList.push_back(itr->path().string() );
            }
            else
            {
                LOG(WARNING) << "invalid format for SCD file name: " << fileName;
            }
        }
    }

    // sort files
    sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);

    return true;
}

void backupSCDFiles(const std::string& scdDir, const std::vector<string>& scdList)
{
    bfs::path bkDir = bfs::path(scdDir) / SCD_BACKUP_DIR;
    bfs::create_directory(bkDir);

    LOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;
    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        try
        {
            bfs::rename(*scdIt, bkDir / bfs::path(*scdIt).filename());
        }
        catch(bfs::filesystem_error& e)
        {
            LOG(WARNING) << "exception in rename file " << *scdIt << ": " << e.what();
        }
    }
}

bool backupDataFiles(sf1r::directory::DirectoryRotator& directoryRotator)
{
    const boost::shared_ptr<sf1r::directory::Directory>& current = directoryRotator.currentDirectory();
    const boost::shared_ptr<sf1r::directory::Directory>& next = directoryRotator.nextDirectory();

    // valid pointer
    // && not the same directory
    // && have not copied successfully yet
    if (next
        && current->name() != next->name()
        && ! (next->valid() && next->parentName() == current->name()))
    {
        try
        {
            LOG(INFO) << "Copy data dir from " << current->name() << " to " << next->name();
            next->copyFrom(*current);
            return true;
        }
        catch(boost::filesystem::filesystem_error& e)
        {
            LOG(INFO) << "exception in copy data dir, " << e.what();
        }

        // try copying but failed
        return false;
    }

    // not copy, always returns true
    return true;
}

bool doc2User(const SCDDoc& doc, sf1r::User& user, const sf1r::RecommendSchema& schema)
{
    for (SCDDoc::const_iterator it = doc.begin(); it != doc.end(); ++it)
    {
        std::string propName;
        it->first.convertString(propName, DEFAULT_ENCODING);
        const izenelib::util::UString & propValueU = it->second;

        if (propName == "USERID")
        {
            propValueU.convertString(user.idStr_, DEFAULT_ENCODING);
        }
        else
        {
            sf1r::RecommendProperty recommendProperty;
            if (schema.getUserProperty(propName, recommendProperty))
            {
                user.propValueMap_[propName] = propValueU;
            }
            else
            {
                LOG(ERROR) << "Unknown user property " + propName + " in SCD file";
                return false;
            }
        }
    }

    if (user.idStr_.empty())
    {
        LOG(ERROR) << "missing user property <USERID> in SCD file";
        return false;
    }

    return true;
}

bool doc2Item(const SCDDoc& doc, sf1r::Item& item, const sf1r::RecommendSchema& schema)
{
    for (SCDDoc::const_iterator it = doc.begin(); it != doc.end(); ++it)
    {
        std::string propName;
        it->first.convertString(propName, DEFAULT_ENCODING);
        const izenelib::util::UString & propValueU = it->second;

        if (propName == "ITEMID")
        {
            propValueU.convertString(item.idStr_, DEFAULT_ENCODING);
        }
        else
        {
            sf1r::RecommendProperty recommendProperty;
            if (schema.getItemProperty(propName, recommendProperty))
            {
                item.propValueMap_[propName] = propValueU;
            }
            else
            {
                LOG(ERROR) << "Unknown item property " + propName + " in SCD file";
                return false;
            }
        }
    }

    if (item.idStr_.empty())
    {
        LOG(ERROR) << "missing item property <ITEMID> in SCD file";
        return false;
    }

    return true;
}

bool doc2Order(
    const SCDDoc& doc,
    std::string& userIdStr,
    std::string& orderIdStr,
    std::string& dateStr,
    sf1r::RecommendTaskService::OrderItem& orderItem
)
{
    std::map<string, string> docMap;
    for (SCDDoc::const_iterator it = doc.begin(); it != doc.end(); ++it)
    {
        std::string propName;
        std::string propValue;
        it->first.convertString(propName, DEFAULT_ENCODING);
        it->second.convertString(docMap[propName], DEFAULT_ENCODING);
    }

    userIdStr = docMap["USERID"];
    if (userIdStr.empty())
    {
        LOG(ERROR) << "missing property <USERID> in order SCD file";
        return false;
    }

    orderItem.itemIdStr_ = docMap["ITEMID"];
    if (orderItem.itemIdStr_.empty())
    {
        LOG(ERROR) << "missing property <ITEMID> in order SCD file";
        return false;
    }

    orderIdStr = docMap["order_id"];
    dateStr = docMap["DATE"];

    if (docMap["quantity"].empty() == false)
    {
        try
        {
            orderItem.quantity_ = boost::lexical_cast<int>(docMap["quantity"]);
        }
        catch(boost::bad_lexical_cast& e)
        {
            LOG(ERROR) << "error in casting quantity " << docMap["quantity"] << " to int value";
        }
    }

    if (docMap["price"].empty() == false)
    {
        try
        {
            orderItem.price_ = boost::lexical_cast<double>(docMap["price"]);
        }
        catch(boost::bad_lexical_cast& e)
        {
            LOG(ERROR) << "error in casting price " << docMap["price"] << " to double value";
        }
    }

    return true;
}

class VisitTask
{
public:
    VisitTask(
        sf1r::VisitManager& visitManager,
        sf1r::userid_t userId,
        sf1r::itemid_t itemId
    )
    : visitManager_(visitManager)
    , userId_(userId)
    , itemId_(itemId)
    {
    }

    void visit()
    {
        visitManager_.addVisitItem(userId_, itemId_);
    }

private:
    sf1r::VisitManager& visitManager_;
    sf1r::userid_t userId_;
    sf1r::itemid_t itemId_;
};

class OrderTask
{
public:
    OrderTask(
        sf1r::OrderManager& orderManager,
        std::vector<sf1r::itemid_t>& orderItems
    )
    : orderManager_(orderManager)
    , orderItems_(orderItems)
    {
    }

    OrderTask(const OrderTask& orderTask)
    : orderManager_(orderTask.orderManager_)
    {
        orderItems_.swap(orderTask.orderItems_);
    }

    void purchase()
    {
        orderManager_.addOrder(orderItems_);
    }

private:
    sf1r::OrderManager& orderManager_;
    mutable std::vector<sf1r::itemid_t> orderItems_;
};

class PurchaseTask
{
public:
    PurchaseTask(
        sf1r::PurchaseManager& purchaseManager,
        sf1r::userid_t userId,
        std::vector<sf1r::itemid_t>& itemVec
    )
    : purchaseManager_(purchaseManager)
    , userId_(userId)
    {
        itemVec_.swap(itemVec);
    }

    PurchaseTask(const PurchaseTask& task)
    : purchaseManager_(task.purchaseManager_)
    , userId_(task.userId_)
    {
        itemVec_.swap(task.itemVec_);
    }

    void purchase()
    {
        purchaseManager_.addPurchaseItem(userId_, itemVec_);
    }

private:
    sf1r::PurchaseManager& purchaseManager_;
    sf1r::userid_t userId_;
    mutable std::vector<sf1r::itemid_t> itemVec_;
};

}

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration* bundleConfig,
    directory::DirectoryRotator* directoryRotator,
    UserManager* userManager,
    ItemManager* itemManager,
    VisitManager* visitManager,
    PurchaseManager* purchaseManager,
    OrderManager* orderManager,
    RecIdGenerator* userIdGenerator,
    RecIdGenerator* itemIdGenerator
)
    :bundleConfig_(bundleConfig)
    ,directoryRotator_(directoryRotator)
    ,userManager_(userManager)
    ,itemManager_(itemManager)
    ,visitManager_(visitManager)
    ,purchaseManager_(purchaseManager)
    ,orderManager_(orderManager)
    ,userIdGenerator_(userIdGenerator)
    ,itemIdGenerator_(itemIdGenerator)
    ,jobScheduler_(new JobScheduler())
{
}

RecommendTaskService::~RecommendTaskService()
{
    delete jobScheduler_;
}

bool RecommendTaskService::addUser(const User& user)
{
    if (user.idStr_.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(user.idStr_, userId))
    {
        LOG(WARNING) << "in addUser(), user id " << user.idStr_ << " already exists";
        // not return false here, if the user id was once removed in userManager_,
        // it would still exists in userIdGenerator_
    }

    return userManager_->addUser(userId, user);
}

bool RecommendTaskService::updateUser(const User& user)
{
    if (user.idStr_.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(user.idStr_, userId, false) == false)
    {
        LOG(ERROR) << "error in updateUser(), user id " << user.idStr_ << " not yet added before";
        return false;
    }

    return userManager_->updateUser(userId, user);
}

bool RecommendTaskService::removeUser(const std::string& userIdStr)
{
    if (userIdStr.empty())
    {
        return false;
    }

    userid_t userId = 0;

    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in removeUser(), user id " << userIdStr << " not yet added before";
        return false;
    }

    return userManager_->removeUser(userId);
}

bool RecommendTaskService::addItem(const Item& item)
{
    if (item.idStr_.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(item.idStr_, itemId))
    {
        LOG(WARNING) << "in addItem(), item id " << item.idStr_ << " already exists";
    }

    return itemManager_->addItem(itemId, item);
}

bool RecommendTaskService::updateItem(const Item& item)
{
    if (item.idStr_.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(item.idStr_, itemId, false) == false)
    {
        LOG(ERROR) << "error in updateItem(), item id " << item.idStr_ << " not yet added before";
        return false;
    }

    return itemManager_->updateItem(itemId, item);
}

bool RecommendTaskService::removeItem(const std::string& itemIdStr)
{
    if (itemIdStr.empty())
    {
        return false;
    }

    itemid_t itemId = 0;

    if (itemIdGenerator_->conv(itemIdStr, itemId, false) == false)
    {
        LOG(ERROR) << "error in removeItem(), item id " << itemIdStr << " not yet added before";
        return false;
    }

    return itemManager_->removeItem(itemId);
}

bool RecommendTaskService::visitItem(const std::string& userIdStr, const std::string& itemIdStr)
{
    if (userIdStr.empty() || itemIdStr.empty())
    {
        return false;
    }

    userid_t userId = 0;
    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in visitItem(), user id " << userIdStr << " not yet added before";
        return false;
    }

    itemid_t itemId = 0;
    if (itemIdGenerator_->conv(itemIdStr, itemId, false) == false)
    {
        LOG(ERROR) << "error in visitItem(), item id " << itemIdStr << " not yet added before";
        return false;
    }

    VisitTask task(*visitManager_, userId, itemId);
    jobScheduler_->addTask(boost::bind(&VisitTask::visit, task));

    return true;
}

bool RecommendTaskService::purchaseItem(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec
)
{
    userid_t userId = 0;
    std::vector<itemid_t> itemIdVec;

    if (convertUserItemId_(userIdStr, orderItemVec, userId, itemIdVec) == false)
    {
        return false;
    }

    {
        PurchaseTask task(*purchaseManager_, userId, itemIdVec);
        jobScheduler_->addTask(boost::bind(&PurchaseTask::purchase, task));
    }

    {
        OrderTask task(*orderManager_, itemIdVec);
        jobScheduler_->addTask(boost::bind(&OrderTask::purchase, task));
    }

    return true;
}

bool RecommendTaskService::buildCollection()
{
    LOG(INFO) << "Start building recommend collection...";
    izenelib::util::ClockTimer timer;

    if(!backupDataFiles(*directoryRotator_))
    {
        LOG(ERROR) << "Failed in backup data files, exit recommend collection build";
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_->currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Dirty recommend collection data, exit recommend collection build";
        return false;
    }

    if (loadUserSCD_() && loadItemSCD_() && loadOrderSCD_())
    {
        LOG(INFO) << "End recommend collection build, elapsed time: " << timer.elapsed() << " seconds";
        return true;
    }

    LOG(ERROR) << "Failed recommend collection build";
    return false;
}

bool RecommendTaskService::loadUserSCD_()
{
    std::string scdDir = bundleConfig_->userSCDPath();
    std::vector<string> scdList;

    if (scanSCDFiles(scdDir, scdList) == false)
    {
        return false;
    }

    if (scdList.empty())
    {
        LOG(WARNING) << "no user SCD files to load";
        return true;
    }

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        ScdParser userParser(DEFAULT_ENCODING, "<USERID>");
        if (userParser.load(*scdIt) == false)
        {
            LOG(ERROR) << "ScdParser failed in loading " << *scdIt;
            continue;
        }

        const SCD_TYPE scdType = ScdParser::checkSCDType(*scdIt);
        if (scdType == NOT_SCD)
        {
            LOG(ERROR) << "Unknown SCD type of file " << *scdIt;
            continue;
        }

        int docNum = 0;
        for (ScdParser::iterator docIter = userParser.begin();
            docIter != userParser.end(); ++docIter)
        {
            ++docNum;

            SCDDocPtr docPtr = (*docIter);
            const SCDDoc& doc = *docPtr;

            User user;
            if (doc2User(doc, user, bundleConfig_->recommendSchema_) == false)
            {
                LOG(ERROR) << "error in parsing User, docNum: " << docNum << ", SCD file: " << *scdIt;
                continue;
            }

            switch(scdType)
            {
            case INSERT_SCD:
                if (addUser(user) == false)
                {
                    LOG(ERROR) << "error in adding User, USERID: " << user.idStr_;
                }
                break;

            case UPDATE_SCD:
                if (updateUser(user) == false)
                {
                    LOG(ERROR) << "error in updating User, USERID: " << user.idStr_;
                }
                break;

            case DELETE_SCD:
                if (removeUser(user.idStr_) == false)
                {
                    LOG(ERROR) << "error in removing User, USERID: " << user.idStr_;
                }
                break;

            default:
                LOG(ERROR) << "unknown SCD type " << scdType << ", SCD file: " << *scdIt;
                break;
            }
        }
    }

    backupSCDFiles(scdDir, scdList);

    return true;
}

bool RecommendTaskService::loadItemSCD_()
{
    std::string scdDir = bundleConfig_->itemSCDPath();
    std::vector<string> scdList;

    if (scanSCDFiles(scdDir, scdList) == false)
    {
        return false;
    }

    if (scdList.empty())
    {
        LOG(WARNING) << "no item SCD files to load";
        return true;
    }

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        ScdParser itemParser(DEFAULT_ENCODING, "<ITEMID>");
        if (itemParser.load(*scdIt) == false)
        {
            LOG(ERROR) << "ScdParser failed in loading " << *scdIt;
            continue;
        }

        const SCD_TYPE scdType = ScdParser::checkSCDType(*scdIt);
        if (scdType == NOT_SCD)
        {
            LOG(ERROR) << "Unknown SCD type of file " << *scdIt;
            continue;
        }

        int docNum = 0;
        for (ScdParser::iterator docIter = itemParser.begin();
            docIter != itemParser.end(); ++docIter)
        {
            ++docNum;

            SCDDocPtr docPtr = (*docIter);
            const SCDDoc& doc = *docPtr;

            Item item;
            if (doc2Item(doc, item, bundleConfig_->recommendSchema_) == false)
            {
                LOG(ERROR) << "error in parsing item, docNum: " << docNum << ", SCD file: " << *scdIt;
                continue;
            }

            switch(scdType)
            {
            case INSERT_SCD:
                if (addItem(item) == false)
                {
                    LOG(ERROR) << "error in adding item, ITEMID: " << item.idStr_;
                }
                break;

            case UPDATE_SCD:
                if (updateItem(item) == false)
                {
                    LOG(ERROR) << "error in updating item, ITEMID: " << item.idStr_;
                }
                break;

            case DELETE_SCD:
                if (removeItem(item.idStr_) == false)
                {
                    LOG(ERROR) << "error in removing item, ITEMID: " << item.idStr_;
                }
                break;

            default:
                LOG(ERROR) << "unknown SCD type " << scdType << ", SCD file: " << *scdIt;
                break;
            }
        }
    }

    backupSCDFiles(scdDir, scdList);

    return true;
}

bool RecommendTaskService::loadOrderSCD_()
{
    std::string scdDir = bundleConfig_->orderSCDPath();
    std::vector<string> scdList;

    if (scanSCDFiles(scdDir, scdList) == false)
    {
        return false;
    }

    if (scdList.empty())
    {
        LOG(WARNING) << "no order SCD files to load";
        return true;
    }

    // clear old file
    std::string mapFileName = directoryRotator_->currentDirectory()->path().string() + "/user_item.sdb.tmp";
    bfs::remove(mapFileName);

    // collect item ids for each user
    UserItemMap userItemMap(mapFileName);
    // adjust the bucket array number in TC hash
    const unsigned int USER_NUM = userManager_->userNum();
    const unsigned int DEFAULT_BNUM = 1 << 17; // default bucket array number
    if (USER_NUM > DEFAULT_BNUM)
    {
        izenelib::am::tc_hash<userid_t, ItemIdSet>& tcHash = userItemMap.getContainer();
        tcHash.tune(USER_NUM, -1, -1, 0);
    }
    userItemMap.open();

    int docNum = 0;
    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        ScdParser orderParser(DEFAULT_ENCODING, "<USERID>");
        if (orderParser.load(*scdIt) == false)
        {
            LOG(ERROR) << "ScdParser failed in loading " << *scdIt;
            continue;
        }

        const SCD_TYPE scdType = ScdParser::checkSCDType(*scdIt);
        if (scdType != INSERT_SCD)
        {
            LOG(ERROR) << "Only insert type is allowed for order SCD file " << *scdIt;
            continue;
        }

        OrderMap orderMap;
        for (ScdParser::iterator docIter = orderParser.begin();
            docIter != orderParser.end(); ++docIter)
        {
            if (++docNum % 10000 == 0)
            {
                std::cout << "\rloading orders, total docNum: " << docNum << ", SCD file: " << *scdIt << std::flush;
            }

            SCDDocPtr docPtr = (*docIter);
            const SCDDoc& doc = *docPtr;

            std::string userIdStr;
            std::string orderIdStr;
            std::string dateStr;
            OrderItem orderItem;
            if (doc2Order(doc, userIdStr, orderIdStr, dateStr, orderItem) == false)
            {
                LOG(ERROR) << "error in parsing Order, SCD file: " << *scdIt
                           << ", userId: " << userIdStr
                           << ", orderId: " << orderIdStr
                           << ", date: " << dateStr;
                continue;
            }

            assert(userIdStr.empty() == false);

            if (orderIdStr.empty())
            {
                OrderItemVec orderItemVec;
                orderItemVec.push_back(orderItem);
                if (loadOrder_(userIdStr, orderItemVec, userItemMap) == false)
                {
                    LOG(ERROR) << "error in loading order, USERID: " << userIdStr
                               << ", order id: " << orderIdStr
                               << ", item num: " << orderItemVec.size();
                }
            }
            else
            {
                OrderKey orderKey(userIdStr, orderIdStr);
                OrderMap::iterator mapIt = orderMap.find(orderKey);
                if (mapIt != orderMap.end())
                {
                    mapIt->second.push_back(orderItem);
                }
                else
                {
                    if (orderMap.size() >= MAX_ORDER_NUM)
                    {
                        loadOrderMap_(orderMap, userItemMap);
                        orderMap.clear();
                    }

                    orderMap[orderKey].push_back(orderItem);
                }
            }
        }

        loadOrderMap_(orderMap, userItemMap);
    }
    std::cout << "\rloading orders, total docNum: " << docNum << std::endl;

    LOG(INFO) << "loading the purchased items for " << userItemMap.numItems() << " users...";
    typedef izenelib::sdb::SDBCursorIterator<UserItemMap> UserItemIterator;
    UserItemIterator itEnd;
    int userNum = 0;
    for (UserItemIterator it = UserItemIterator(userItemMap); it != itEnd; ++it)
    { 
        if (++userNum % 100 == 0)
        {
            std::cout << "\rloading user num: " << userNum << std::flush;
        }

        std::vector<itemid_t> itemVec(it->second.begin(), it->second.end());
        if (purchaseManager_->addPurchaseItem(it->first, itemVec) == false)
        {
            LOG(ERROR) << "error in PurchaseManager::addPurchaseItem(), user id: " << it->first
                       << ", item num: " << itemVec.size();
        }
    }
    std::cout << "\rloading user num: " << userNum << std::endl;

    backupSCDFiles(scdDir, scdList);

    return true;
}

void RecommendTaskService::loadOrderMap_(
    const OrderMap& orderMap,
    UserItemMap& userItemMap
)
{
    for (OrderMap::const_iterator it = orderMap.begin(); it != orderMap.end(); ++it)
    {
        if (loadOrder_(it->first.first, it->second, userItemMap) == false)
        {
            LOG(ERROR) << "error in adding order, USERID: " << it->first.first
                       << ", order id: " << it->first.second
                       << ", item num: " << it->second.size();
        }
    }
}

bool RecommendTaskService::loadOrder_(
    const std::string& userIdStr,
    const OrderItemVec& orderItemVec,
    UserItemMap& userItemMap
)
{
    userid_t userId = 0;
    std::vector<itemid_t> itemIdVec;

    if (convertUserItemId_(userIdStr, orderItemVec, userId, itemIdVec) == false)
    {
        return false;
    }

    orderManager_->addOrder(itemIdVec);

    // collect items for each user
    ItemIdSet itemIdSet;
    userItemMap.getValue(userId, itemIdSet);
    bool needUpdate = false;
    for (std::vector<itemid_t>::const_iterator it = itemIdVec.begin();
        it != itemIdVec.end(); ++it)
    {
        if (itemIdSet.insert(*it).second && needUpdate == false)
        {
            needUpdate = true;
        }
    }

    // not purchased yet
    if (needUpdate)
    {
        try
        {
            if (userItemMap.update(userId, itemIdSet) == false)
            {
                return false;
            }
        }
        catch(izenelib::util::IZENELIBException& e)
        {
            LOG(ERROR) << "exception in SDB::update(): " << e.what();
        }
    }

    return true;
}

bool RecommendTaskService::convertUserItemId_(
    const std::string& userIdStr,
    const OrderItemVec& orderItemVec,
    userid_t& userId,
    std::vector<itemid_t>& itemIdVec
)
{
    if (userIdStr.empty() || orderItemVec.empty())
    {
        return false;
    }

    if (userIdGenerator_->conv(userIdStr, userId, false) == false)
    {
        LOG(ERROR) << "error in convertUserItemId(), user id " << userIdStr << " not yet added before";
        return false;
    }

    for (OrderItemVec::const_iterator it = orderItemVec.begin();
        it != orderItemVec.end(); ++it)
    {
        if (it->itemIdStr_.empty())
        {
            return false;
        }

        itemid_t itemId = 0;
        if (itemIdGenerator_->conv(it->itemIdStr_, itemId, false) == false)
        {
            LOG(ERROR) << "error in convertUserItemId(), item id " << it->itemIdStr_ << " not yet added before";
            return false;
        }

        itemIdVec.push_back(itemId);
    }

    return true;
}

} // namespace sf1r

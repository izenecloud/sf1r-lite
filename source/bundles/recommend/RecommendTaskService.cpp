#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/User.h>
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/ItemManager.h>
#include <recommend-manager/storage/VisitManager.h>
#include <recommend-manager/storage/PurchaseManager.h>
#include <recommend-manager/storage/CartManager.h>
#include <recommend-manager/OrderManager.h>
#include <recommend-manager/EventManager.h>
#include <recommend-manager/RateManager.h>
#include <recommend-manager/RateParam.h>
#include <recommend-manager/ItemIdGenerator.h>
#include <log-manager/OrderLogger.h>
#include <log-manager/ItemLogger.h>
#include <common/ScdParser.h>
#include <directory-manager/Directory.h>
#include <directory-manager/DirectoryRotator.h>

#include <sdb/SDBCursorIterator.h>
#include <util/scheduler.h>

#include <map>
#include <set>
#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace
{
/** the default encoding type */
const izenelib::util::UString::EncodingType DEFAULT_ENCODING = izenelib::util::UString::UTF_8;

const char* SCD_DELIM = "<USERID>";

const char* PROP_USERID = "USERID";
const char* PROP_ITEMID = "ITEMID";
const char* PROP_ORDERID = "order_id";
const char* PROP_DATE = "DATE";
const char* PROP_QUANTITY = "quantity";
const char* PROP_PRICE = "price";

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
    LOG(INFO) << "scanning SCD files in " << scdDir;
    const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdDir); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename().string();

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
    if (scdList.empty() == false)
    {
        LOG(INFO) << "sorting " << scdList.size() << " SCD file names...";
        sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);
    }

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

        if (propName == PROP_USERID)
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
        LOG(ERROR) << "missing user property <" << PROP_USERID << "> in SCD file";
        return false;
    }

    return true;
}

bool doc2Order(
    const SCDDoc& doc,
    std::string& userIdStr,
    std::string& orderIdStr,
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

    userIdStr = docMap[PROP_USERID];
    if (userIdStr.empty())
    {
        LOG(ERROR) << "missing property <" << PROP_USERID << "> in order SCD file";
        return false;
    }

    orderItem.itemIdStr_ = docMap[PROP_ITEMID];
    if (orderItem.itemIdStr_.empty())
    {
        LOG(ERROR) << "missing property <" << PROP_ITEMID << "> in order SCD file";
        return false;
    }

    orderIdStr = docMap[PROP_ORDERID];
    orderItem.dateStr_ = docMap[PROP_DATE];

    if (docMap[PROP_QUANTITY].empty() == false)
    {
        try
        {
            orderItem.quantity_ = boost::lexical_cast<int>(docMap[PROP_QUANTITY]);
        }
        catch(boost::bad_lexical_cast& e)
        {
            LOG(WARNING) << "error in casting quantity " << docMap[PROP_QUANTITY] << " to int value";
        }
    }

    if (docMap[PROP_PRICE].empty() == false)
    {
        try
        {
            orderItem.price_ = boost::lexical_cast<double>(docMap[PROP_PRICE]);
        }
        catch(boost::bad_lexical_cast& e)
        {
            LOG(WARNING) << "error in casting price " << docMap[PROP_PRICE] << " to double value";
        }
    }

    return true;
}

}

namespace sf1r
{

RecommendTaskService::RecommendTaskService(
    RecommendBundleConfiguration& bundleConfig,
    directory::DirectoryRotator& directoryRotator,
    UserManager& userManager,
    ItemManager& itemManager,
    VisitManager& visitManager,
    PurchaseManager& purchaseManager,
    CartManager& cartManager,
    OrderManager& orderManager,
    EventManager& eventManager,
    RateManager& rateManager,
    ItemIdGenerator& itemIdGenerator,
    CoVisitManager& coVisitManager,
    ItemCFManager& itemCFManager
)
    :bundleConfig_(bundleConfig)
    ,directoryRotator_(directoryRotator)
    ,userManager_(userManager)
    ,itemManager_(itemManager)
    ,visitManager_(visitManager)
    ,purchaseManager_(purchaseManager)
    ,cartManager_(cartManager)
    ,orderManager_(orderManager)
    ,eventManager_(eventManager)
    ,rateManager_(rateManager)
    ,itemIdGenerator_(itemIdGenerator)
    ,coVisitManager_(coVisitManager)
    ,itemCFManager_(itemCFManager)
    ,visitMatrix_(coVisitManager_)
    ,purchaseMatrix_(itemCFManager_)
    ,purchaseCoVisitMatrix_(itemCFManager_)
    ,cronJobName_("RecommendTaskService-" + bundleConfig.collectionName_)
{
    if (cronExpression_.setExpression(bundleConfig_.cronStr_))
    {
        bool result = izenelib::util::Scheduler::addJob(cronJobName_,
                                                        60*1000, // each minute
                                                        0, // start from now
                                                        boost::bind(&RecommendTaskService::cronJob_, this));
        if (!result)
        {
            LOG(ERROR) << "failed in izenelib::util::Scheduler::addJob(), cron job name: " << cronJobName_;
        }
    }

    initProps_();
}

RecommendTaskService::~RecommendTaskService()
{
    izenelib::util::Scheduler::removeJob(cronJobName_);
}

void RecommendTaskService::initProps_()
{
    const std::vector<RecommendProperty>& userSchema = bundleConfig_.recommendSchema_.userSchema_;
    for (std::vector<RecommendProperty>::const_iterator it = userSchema.begin();
        it != userSchema.end(); ++it)
    {
        userProps_.push_back(it->propertyName_);
    }
    userProps_.push_back(PROP_USERID);

    orderProps_.push_back(PROP_USERID);
    orderProps_.push_back(PROP_ITEMID);
    orderProps_.push_back(PROP_ORDERID);
    orderProps_.push_back(PROP_DATE);
    orderProps_.push_back(PROP_QUANTITY);
    orderProps_.push_back(PROP_PRICE);
}

bool RecommendTaskService::addUser(const User& user)
{
    return userManager_.addUser(user);
}

bool RecommendTaskService::updateUser(const User& user)
{
    return userManager_.updateUser(user);
}

bool RecommendTaskService::removeUser(const std::string& userIdStr)
{
    return userManager_.removeUser(userIdStr);
}

bool RecommendTaskService::visitItem(
    const std::string& sessionIdStr,
    const std::string& userIdStr,
    const std::string& itemIdStr,
    bool isRecItem
)
{
    if (sessionIdStr.empty())
    {
        LOG(ERROR) << "error in visitItem(), session id is empty";
        return false;
    }

    itemid_t itemId = 0;
    if (! itemIdGenerator_.getItemIdByStrId(itemIdStr, itemId))
        return false;

    jobScheduler_.addTask(boost::bind(&VisitManager::addVisitItem, &visitManager_,
                                      sessionIdStr, userIdStr, itemId, &visitMatrix_));

    if (isRecItem && !visitManager_.visitRecommendItem(userIdStr, itemId))
    {
        LOG(ERROR) << "error in VisitManager::visitRecommendItem(), userId: " << userIdStr
                   << ", itemId: " << itemId;
        return false;
    }

    return true;
}

bool RecommendTaskService::purchaseItem(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec
)
{
    jobScheduler_.addTask(boost::bind(&RecommendTaskService::saveOrder_, this,
                                       userIdStr, orderIdStr, orderItemVec, &purchaseMatrix_));

    return true;
}

bool RecommendTaskService::updateShoppingCart(
    const std::string& userIdStr,
    const OrderItemVec& cartItemVec
)
{
    std::vector<itemid_t> itemIdVec;
    if (! convertOrderItemVec_(cartItemVec, itemIdVec))
        return false;

    return cartManager_.updateCart(userIdStr, itemIdVec);
}

bool RecommendTaskService::trackEvent(
    bool isAdd,
    const std::string& eventStr,
    const std::string& userIdStr,
    const std::string& itemIdStr
)
{
    itemid_t itemId = 0;
    if (! itemIdGenerator_.getItemIdByStrId(itemIdStr, itemId))
        return false;

    return isAdd ? eventManager_.addEvent(eventStr, userIdStr, itemId) :
                   eventManager_.removeEvent(eventStr, userIdStr, itemId);
}

bool RecommendTaskService::rateItem(const RateParam& param)
{
    itemid_t itemId = 0;
    if (! itemIdGenerator_.getItemIdByStrId(param.itemIdStr, itemId))
        return false;

    return param.isAdd ? rateManager_.addRate(param.userIdStr, itemId, param.rate) :
                         rateManager_.removeRate(param.userIdStr, itemId);
}

void RecommendTaskService::buildCollection()
{
    jobScheduler_.addTask(boost::bind(&RecommendTaskService::buildCollectionImpl_, this));
}

bool RecommendTaskService::loadUserSCD_()
{
    std::string scdDir = bundleConfig_.userSCDPath();
    std::vector<string> scdList;

    if (scanSCDFiles(scdDir, scdList) == false)
        return false;

    if (scdList.empty())
        return true;

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        parseUserSCD_(*scdIt);
    }

    userManager_.flush();

    backupSCDFiles(scdDir, scdList);

    return true;
}

bool RecommendTaskService::parseUserSCD_(const std::string& scdPath)
{
    LOG(INFO) << "parsing SCD file: " << scdPath;

    ScdParser userParser(DEFAULT_ENCODING, SCD_DELIM);
    if (userParser.load(scdPath) == false)
    {
        LOG(ERROR) << "ScdParser loading failed";
        return false;
    }

    const SCD_TYPE scdType = ScdParser::checkSCDType(scdPath);
    if (scdType == NOT_SCD)
    {
        LOG(ERROR) << "Unknown SCD type";
        return false;
    }

    int userNum = 0;
    for (ScdParser::iterator docIter = userParser.begin(userProps_);
        docIter != userParser.end(); ++docIter)
    {
        if (++userNum % 10000 == 0)
        {
            std::cout << "\rloading user num: " << userNum << "\t" << std::flush;
        }

        SCDDocPtr docPtr = (*docIter);
        const SCDDoc& doc = *docPtr;

        User user;
        if (doc2User(doc, user, bundleConfig_.recommendSchema_) == false)
        {
            LOG(ERROR) << "error in parsing User, userNum: " << userNum;
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
            LOG(ERROR) << "unknown SCD type " << scdType;
            break;
        }

        // terminate execution if interrupted
        boost::this_thread::interruption_point();
    }

    std::cout << "\rloading user num: " << userNum << "\t" << std::endl;

    return true;
}

bool RecommendTaskService::loadOrderSCD_()
{
    std::string scdDir = bundleConfig_.orderSCDPath();
    std::vector<string> scdList;

    if (scanSCDFiles(scdDir, scdList) == false)
        return false;

    if (scdList.empty())
        return true;

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        parseOrderSCD_(*scdIt);
    }

    orderManager_.flush();
    purchaseManager_.flush();

    itemCFManager_.buildSimMatrix();
    itemCFManager_.flush();

    buildFreqItemSet_();

    backupSCDFiles(scdDir, scdList);

    return true;
}

bool RecommendTaskService::parseOrderSCD_(const std::string& scdPath)
{
    LOG(INFO) << "parsing SCD file: " << scdPath;

    ScdParser orderParser(DEFAULT_ENCODING, SCD_DELIM);
    if (orderParser.load(scdPath) == false)
    {
        LOG(ERROR) << "ScdParser loading failed";
        return false;
    }

    const SCD_TYPE scdType = ScdParser::checkSCDType(scdPath);
    if (scdType != INSERT_SCD)
    {
        LOG(ERROR) << "Only insert type is allowed for order SCD file";
        return false;
    }

    int orderNum = 0;
    OrderMap orderMap;
    for (ScdParser::iterator docIter = orderParser.begin(orderProps_);
        docIter != orderParser.end(); ++docIter)
    {
        if (++orderNum % 10000 == 0)
        {
            std::cout << "\rloading order[" << orderNum << "], "
                      << itemCFManager_ << std::flush;
        }

        SCDDocPtr docPtr = (*docIter);
        const SCDDoc& doc = *docPtr;

        std::string userIdStr;
        std::string orderIdStr;
        OrderItem orderItem;

        if (doc2Order(doc, userIdStr, orderIdStr, orderItem) == false)
        {
            LOG(ERROR) << "error in parsing Order SCD file";
            continue;
        }

        loadOrderItem_(userIdStr, orderIdStr, orderItem, orderMap);

        // terminate execution if interrupted
        boost::this_thread::interruption_point();
    }

    saveOrderMap_(orderMap);
    std::cout << "\rloading order[" << orderNum << "], "
              << itemCFManager_ << std::endl;

    return true;
}

void RecommendTaskService::loadOrderItem_(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItem& orderItem,
    OrderMap& orderMap
)
{
    assert(userIdStr.empty() == false);

    if (orderIdStr.empty())
    {
        OrderItemVec orderItemVec;
        orderItemVec.push_back(orderItem);
        saveOrder_(userIdStr, orderIdStr, orderItemVec, &purchaseCoVisitMatrix_);
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
                saveOrderMap_(orderMap);
                orderMap.clear();
            }

            orderMap[orderKey].push_back(orderItem);
        }
    }
}

void RecommendTaskService::saveOrderMap_(const OrderMap& orderMap)
{
    for (OrderMap::const_iterator it = orderMap.begin(); it != orderMap.end(); ++it)
    {
        saveOrder_(it->first.first, it->first.second, it->second, &purchaseCoVisitMatrix_);
    }
}

bool RecommendTaskService::saveOrder_(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec,
    RecommendMatrix* matrix
)
{
    if (orderItemVec.empty())
    {
        LOG(WARNING) << "empty order in RecommendTaskService::saveOrder_()";
        return false;
    }

    std::vector<itemid_t> itemIdVec;
    if (! convertOrderItemVec_(orderItemVec, itemIdVec))
        return false;

    orderManager_.addOrder(itemIdVec);

    if (purchaseManager_.addPurchaseItem(userIdStr, itemIdVec, matrix) &&
        insertOrderDB_(userIdStr, orderIdStr, orderItemVec, itemIdVec))
    {
        return true;
    }

    LOG(ERROR) << "failed in saveOrder_(), USERID: " << userIdStr
               << ", order id: " << orderIdStr
               << ", item num: " << itemIdVec.size();
    return false;
}

bool RecommendTaskService::insertOrderDB_(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec,
    const std::vector<itemid_t>& itemIdVec
)
{
    assert(orderItemVec.empty() == false);

    ItemIdSet recItemSet;
    if (!visitManager_.getRecommendItemSet(userIdStr, recItemSet))
    {
        LOG(ERROR) << "error in VisitManager::getRecItemSet(), user id: " << userIdStr;
        return false;
    }

    OrderLogger& orderLogger = OrderLogger::instance();
    ItemLogger& itemLogger = ItemLogger::instance();
    int orderId = 0;
    if (!orderLogger.insertOrder(orderIdStr, bundleConfig_.collectionName_,
                                 userIdStr, orderItemVec[0].dateStr_, orderId))
    {
        return false;
    }

    bool result = true;
    const unsigned int itemNum = orderItemVec.size();
    for (unsigned int i = 0; i < itemNum; ++i)
    {
        itemid_t itemId = itemIdVec[i];
        bool isRecItem = (recItemSet.find(itemId) != recItemSet.end());
        const RecommendTaskService::OrderItem& orderItem = orderItemVec[i];

        if(!itemLogger.insertItem(orderId, orderItem.itemIdStr_,
                                  orderItem.price_, orderItem.quantity_, isRecItem))
        {
            result = false;
        }
    }

    return result;
}

bool RecommendTaskService::convertOrderItemVec_(
    const OrderItemVec& orderItemVec,
    std::vector<itemid_t>& itemIdVec
)
{
    for (OrderItemVec::const_iterator it = orderItemVec.begin();
        it != orderItemVec.end(); ++it)
    {
        itemid_t itemId = 0;
        if (! itemIdGenerator_.getItemIdByStrId(it->itemIdStr_, itemId))
            return false;

        itemIdVec.push_back(itemId);
    }

    assert(orderItemVec.size() == itemIdVec.size());

    return true;
}

void RecommendTaskService::buildFreqItemSet_()
{
    if (! bundleConfig_.freqItemSetEnable_)
        return;

    LOG(INFO) << "start building frequent item set for collection " << bundleConfig_.collectionName_;

    orderManager_.buildFreqItemsets();

    LOG(INFO) << "finish building frequent item set for collection " << bundleConfig_.collectionName_;
}

void RecommendTaskService::cronJob_()
{
    if (cronExpression_.matches_now())
    {
        jobScheduler_.addTask(boost::bind(&RecommendTaskService::flush_, this));
        jobScheduler_.addTask(boost::bind(&RecommendTaskService::buildFreqItemSet_, this));
    }
}

void RecommendTaskService::flush_()
{
    LOG(INFO) << "start flushing recommend data for collection " << bundleConfig_.collectionName_;

    userManager_.flush();
    visitManager_.flush();
    purchaseManager_.flush();
    cartManager_.flush();
    orderManager_.flush();
    eventManager_.flush();
    rateManager_.flush();

    coVisitManager_.flush();
    itemCFManager_.flush();

    LOG(INFO) << "flushed [Visit] " << coVisitManager_.matrix();
    LOG(INFO) << "flushed [Purchase] " << itemCFManager_;
    LOG(INFO) << "finish flushing recommend data for collection " << bundleConfig_.collectionName_;
}

bool RecommendTaskService::buildCollectionImpl_()
{
    LOG(INFO) << "Start building recommend collection...";
    izenelib::util::ClockTimer timer;

    if(!backupDataFiles(directoryRotator_))
    {
        LOG(ERROR) << "Failed in backup data files, exit recommend collection build";
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Dirty recommend collection data, exit recommend collection build";
        return false;
    }

    if (loadUserSCD_() && loadOrderSCD_())
    {
        LOG(INFO) << "End recommend collection build, elapsed time: " << timer.elapsed() << " seconds";
        return true;
    }

    LOG(ERROR) << "Failed recommend collection build";
    return false;
}

} // namespace sf1r

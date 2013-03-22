#include "RecommendTaskService.h"
#include "RecommendBundleConfiguration.h"
#include <recommend-manager/common/User.h>
#include <recommend-manager/common/RateParam.h>
#include <recommend-manager/item/ItemManager.h>
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/storage/UserManager.h>
#include <recommend-manager/storage/VisitManager.h>
#include <recommend-manager/storage/PurchaseManager.h>
#include <recommend-manager/storage/CartManager.h>
#include <recommend-manager/storage/RateManager.h>
#include <recommend-manager/storage/EventManager.h>
#include <recommend-manager/storage/OrderManager.h>
#include <aggregator-manager/UpdateRecommendBase.h>
#include <aggregator-manager/UpdateRecommendWorker.h>
#include <log-manager/OrderLogger.h>
#include <log-manager/ItemLogger.h>
#include <common/ScdParser.h>
#include <directory-manager/Directory.h>
#include <directory-manager/DirectoryRotator.h>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeFileSyncMgr.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <util/driver/Request.h>

#include <sdb/SDBCursorIterator.h>
#include <util/scheduler.h>

#include <map>
#include <set>
#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

namespace bfs = boost::filesystem;
using namespace izenelib::driver;

namespace
{
/** the default encoding type */
const izenelib::util::UString::EncodingType DEFAULT_ENCODING = izenelib::util::UString::UTF_8;

const char* SCD_DELIM = "<USERID>";

const char* PROP_USERID = "USERID";
const char* PROP_ITEMID = "ITEMID";
const char* PROP_ORDERID = "ORDERID";
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
        const std::string& propName = it->first;
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
        const std::string& propName = it->first;
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
    QueryPurchaseCounter& queryPurchaseCounter,
    UpdateRecommendBase& updateRecommendBase,
    UpdateRecommendWorker* updateRecommendWorker
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
    ,queryPurchaseCounter_(queryPurchaseCounter)
    ,updateRecommendBase_(updateRecommendBase)
    ,updateRecommendWorker_(updateRecommendWorker)
    ,visitMatrix_(updateRecommendBase)
    ,purchaseMatrix_(updateRecommendBase)
    ,purchaseCoVisitMatrix_(updateRecommendBase)
    ,cronJobName_("RecommendTaskService-" + bundleConfig.collectionName_)
{
    if (cronExpression_.setExpression(bundleConfig_.cronStr_))
    {
        bool result = izenelib::util::Scheduler::addJob(cronJobName_,
                                                        60*1000, // each minute
                                                        0, // start from now
                                                        boost::bind(&RecommendTaskService::cronJob_, this, _1));
        if (! result)
        {
            LOG(ERROR) << "failed in izenelib::util::Scheduler::addJob(), cron job name: " << cronJobName_;
        }
    }
}

RecommendTaskService::~RecommendTaskService()
{
    izenelib::util::Scheduler::removeJob(cronJobName_);
}

bool RecommendTaskService::addUser(const User& user)
{
    bool result = userManager_.addUser(user);
    return result;
}

bool RecommendTaskService::updateUser(const User& user)
{
    bool result = userManager_.updateUser(user);
    return result;
}

bool RecommendTaskService::removeUser(const std::string& userIdStr)
{
    bool result = userManager_.removeUser(userIdStr);
    return result;
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
    if (!itemIdGenerator_.strIdToItemId(itemIdStr, itemId))
    {
        return false;
    }

    return visitItemFunc(sessionIdStr, userIdStr, itemId, isRecItem);
}

bool RecommendTaskService::visitItemFunc(
    const std::string& sessionIdStr,
    const std::string& userIdStr,
    itemid_t itemId,
    bool isRecItem
)
{
    // this need to be async because there will be rpc call in visitManager_.addVisitItem
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    RecommendVisitItemReqLog reqlog;
    reqlog.itemid = itemId;
    if(!DistributeRequestHooker::get()->prepare(Req_Recommend_VisitItem, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    if (!visitManager_.addVisitItem(sessionIdStr, userIdStr, itemId, &visitMatrix_))
    {
        return false;
    }

    if (isRecItem && !visitManager_.visitRecommendItem(userIdStr, itemId))
    {
        LOG(ERROR) << "error in VisitManager::visitRecommendItem(), userId: " << userIdStr
            << ", itemId: " << itemId;
        return false;
    }

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}


bool RecommendTaskService::purchaseItem(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec
)
{
    std::vector<itemid_t> itemIdVec;
    if (!prepareSaveOrder_(userIdStr, orderIdStr, orderItemVec, &purchaseMatrix_, itemIdVec))
    {
        LOG(WARNING) << "saveOrder_ failed.";
        return false;
    }

    return purchaseItemFunc(userIdStr, orderIdStr, orderItemVec, itemIdVec);
}

bool RecommendTaskService::purchaseItemFunc(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec,
    const std::vector<itemid_t>& itemIdVec
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = saveOrder_(userIdStr, orderIdStr, orderItemVec, &purchaseMatrix_, itemIdVec);

    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool RecommendTaskService::updateShoppingCart(
    const std::string& userIdStr,
    const OrderItemVec& cartItemVec
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    std::vector<itemid_t> itemIdVec;
    if (! convertOrderItemVec_(cartItemVec, itemIdVec))
    {
        return false;
    }

    if (!bundleConfig_.cassandraConfig_.enable)
    {
        NoAdditionNoRollbackReqLog reqlog;
        if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
        {
            LOG(ERROR) << "prepare failed in " << __FUNCTION__;
            return false;
        }
    }
    bool ret = true;
    bool need_write = !bundleConfig_.cassandraConfig_.enable;
    if (!DistributeRequestHooker::get()->isHooked() || DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
    {
        need_write = true;
    }

    if (need_write)
        ret = cartManager_.updateCart(userIdStr, itemIdVec);
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool RecommendTaskService::trackEvent(
    bool isAdd,
    const std::string& eventStr,
    const std::string& userIdStr,
    const std::string& itemIdStr
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    itemid_t itemId = 0;
    if (! itemIdGenerator_.strIdToItemId(itemIdStr, itemId))
    {
        return false;
    }

    if (!bundleConfig_.cassandraConfig_.enable)
    {
        NoAdditionReqLog reqlog;
        if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
        {
            LOG(ERROR) << "prepare failed in " << __FUNCTION__;
            return false;
        }
    }
    bool ret = true;
    bool need_write = !bundleConfig_.cassandraConfig_.enable;
    if (!DistributeRequestHooker::get()->isHooked() || DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
    {
        need_write = true;
    }

    if (need_write)
    {
        ret = isAdd ? eventManager_.addEvent(eventStr, userIdStr, itemId) :
            eventManager_.removeEvent(eventStr, userIdStr, itemId);
    }
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool RecommendTaskService::rateItem(const RateParam& param)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    itemid_t itemId = 0;
    if (! itemIdGenerator_.strIdToItemId(param.itemIdStr, itemId))
    {
        return false;
    }
    if (!bundleConfig_.cassandraConfig_.enable)
    {
        NoAdditionReqLog reqlog;
        if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
        {
            LOG(ERROR) << "prepare failed in " << __FUNCTION__;
            return false;
        }
    }
    bool ret = true;
    bool need_write = !bundleConfig_.cassandraConfig_.enable;
    if (!DistributeRequestHooker::get()->isHooked() || DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
    {
        need_write = true;
    }

    if (need_write)
    {
        ret = param.isAdd ? rateManager_.addRate(param.userIdStr, itemId, param.rate) :
            rateManager_.removeRate(param.userIdStr, itemId);
    }

    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool RecommendTaskService::buildCollection()
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    LOG(INFO) << "Start building recommend collection...";
    izenelib::util::ClockTimer timer;

    if (! backupDataFiles(directoryRotator_))
    {
        LOG(ERROR) << "Failed in backup data files, exit recommend collection build";
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (! dirGuard)
    {
        LOG(ERROR) << "Dirty recommend collection data, exit recommend collection build";
        return false;
    }

    boost::mutex::scoped_lock lock(buildCollectionMutex_);
    bool ret = false;

    if (!DistributeRequestHooker::get()->isHooked() || DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
    {
        // on primary
        ret = buildCollectionOnPrimary();
    }
    else
    {
        ret = buildCollectionOnReplica();
    }
    if (ret)
    {
        LOG(INFO) << "End recommend collection build, elapsed time: " << timer.elapsed() << " seconds";
    }
    else
    {
        LOG(INFO) << "recommend collection build failed: " ;
    }
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool RecommendTaskService::buildCollectionOnPrimary()
{
    RecommendIndexReqLog reqlog;
    if (!getBuildingSCDFiles(reqlog.user_scd_list, reqlog.order_scd_list))
    {
        return false;
    }
    // push scd files to replicas
    std::vector<std::string> all_scdList = reqlog.user_scd_list;
    all_scdList.insert(all_scdList.end(), reqlog.order_scd_list.begin(), reqlog.order_scd_list.end());

    for (size_t file_index = 0; file_index < all_scdList.size(); ++file_index)
    {
        if(DistributeFileSyncMgr::get()->pushFileToAllReplicas(all_scdList[file_index],
                all_scdList[file_index]))
        {
            LOG(INFO) << "Transfer recommend scd to the replicas finished for: " << all_scdList[file_index];
        }
        else
        {
            LOG(WARNING) << "push recommend scd file to the replicas failed for:" << all_scdList[file_index];
        }
    }

    if (!DistributeRequestHooker::get()->prepare(Req_Recommend_Index, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainMiddle);
    if (loadUserSCD_(reqlog.user_scd_list) && loadOrderSCD_(reqlog.order_scd_list))
    {
        DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
        return true;
    }
    DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
    return false;
}

bool RecommendTaskService::buildCollectionOnReplica()
{
    // build on replicas, using the data from primary.
    RecommendIndexReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_Recommend_Index, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    std::vector<std::string> all_scdList = reqlog.user_scd_list;
    all_scdList.insert(all_scdList.end(), reqlog.order_scd_list.begin(), reqlog.order_scd_list.end());

    LOG(INFO) << "got recommend from primary/log, scd list is : ";
    for (size_t i = 0; i < all_scdList.size(); ++i)
    {
        LOG(INFO) << all_scdList[i];
        bfs::path backup_scd = bfs::path(all_scdList[i]);
        backup_scd = backup_scd.parent_path()/bfs::path(SCD_BACKUP_DIR)/backup_scd.filename();
        if (bfs::exists(all_scdList[i]))
        {
            LOG(INFO) << "found recommend scd file in current directory.";
        }
        else if (bfs::exists(backup_scd))
        {
            // try to find in backup
            LOG(INFO) << "found recommend scd file in backup, move to current";
            bfs::rename(backup_scd, all_scdList[i]);
        }
        else if (!DistributeFileSyncMgr::get()->getFileFromOther(all_scdList[i]))
        {
            if (!DistributeFileSyncMgr::get()->getFileFromOther(backup_scd.string()))
            {
                LOG(INFO) << "recommend scd file missing and get from other failed." << all_scdList[i];
                throw std::runtime_error("recommend scd file missing!");
            }
            else
            {
                LOG(INFO) << "get recommend scd file from other's backup, move to index";
                bfs::rename(backup_scd, all_scdList[i]);
            }
        }
    }


    DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainMiddle);
    bool ret = true;
    // on replica
    // if remote storage enabled, replica can get part of data from remote storage.
    if (!bundleConfig_.cassandraConfig_.enable)
    {
        ret = loadUserSCD_(reqlog.user_scd_list);
        LOG(INFO) << "the recommend did not configure as remote storage, so need to do some write on replicas.";
    }
    if (ret && loadOrderSCD_(reqlog.order_scd_list))
    {
        DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
        return true;
    }
    DistributeRequestHooker::get()->setChainStatus(DistributeRequestHooker::ChainEnd);
    return false;
}

bool RecommendTaskService::getBuildingSCDFiles(std::vector<string>& user_scd_list,
    std::vector<std::string>& order_scd_list)
{
    std::string scdDir = bundleConfig_.userSCDPath();
    if (!scanSCDFiles(scdDir, user_scd_list))
        return false;

    scdDir = bundleConfig_.orderSCDPath();
    if (!scanSCDFiles(scdDir, order_scd_list))
        return false;

    return true;
}

bool RecommendTaskService::loadUserSCD_(const std::vector<string>& scdList)
{
    if (scdList.empty())
        return true;

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        parseUserSCD_(*scdIt);
    }

    userManager_.flush();

    std::string scdDir = bundleConfig_.userSCDPath();
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
    for (ScdParser::iterator docIter = userParser.begin();
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

bool RecommendTaskService::loadOrderSCD_(const std::vector<string> &scdList)
{
    if (scdList.empty())
        return true;

    for (std::vector<string>::const_iterator scdIt = scdList.begin();
        scdIt != scdList.end(); ++scdIt)
    {
        parseOrderSCD_(*scdIt);
    }

    orderManager_.flush();
    purchaseManager_.flush();

    buildFreqItemSet_();

    bool result = true;
    updateRecommendBase_.buildPurchaseSimMatrix(result);
    updateRecommendBase_.flushRecommendMatrix(result);

    std::string scdDir = bundleConfig_.orderSCDPath();
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
    for (ScdParser::iterator docIter = orderParser.begin();
        docIter != orderParser.end(); ++docIter)
    {
        if (updateRecommendWorker_ && ++orderNum % 10000 == 0)
        {
            std::cout << "\rloading order[" << orderNum << "], "
                      << updateRecommendWorker_->itemCFManager() << std::flush;
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

    if (updateRecommendWorker_)
    {
        std::cout << "\rloading order[" << orderNum << "], "
                  << updateRecommendWorker_->itemCFManager() << std::endl;
    }

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
        std::vector<itemid_t> itemIdVec;
        if (prepareSaveOrder_(userIdStr, orderIdStr, orderItemVec, &purchaseCoVisitMatrix_, itemIdVec))
        {
            saveOrder_(userIdStr, orderIdStr, orderItemVec, &purchaseCoVisitMatrix_, itemIdVec);
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
                saveOrderMap_(orderMap);
                orderMap.clear();
            }

            orderMap[orderKey].push_back(orderItem);
        }
    }
}

void RecommendTaskService::saveOrderMap_(const OrderMap& orderMap)
{
    std::vector<itemid_t> itemIdVec;
    for (OrderMap::const_iterator it = orderMap.begin(); it != orderMap.end(); ++it)
    {
        if (prepareSaveOrder_(it->first.first, it->first.second, it->second, & purchaseCoVisitMatrix_, itemIdVec))
        {
            saveOrder_(it->first.first, it->first.second, it->second, &purchaseCoVisitMatrix_, itemIdVec);
        }
        std::vector<itemid_t>().swap(itemIdVec);
    }
}

bool RecommendTaskService::prepareSaveOrder_(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec,
    RecommendMatrix* matrix,
    std::vector<itemid_t>& itemIdVec
    )
{
    if (orderItemVec.empty())
    {
        LOG(WARNING) << "empty order in " << __FUNCTION__;
        return false;
    }

    if (! convertOrderItemVec_(orderItemVec, itemIdVec))
        return false;
    return true;
}

bool RecommendTaskService::saveOrder_(
    const std::string& userIdStr,
    const std::string& orderIdStr,
    const OrderItemVec& orderItemVec,
    RecommendMatrix* matrix,
    const std::vector<itemid_t>& itemIdVec
)
{
    orderManager_.addOrder(itemIdVec);

    if (purchaseManager_.addPurchaseItem(userIdStr, itemIdVec, matrix) &&
        insertPurchaseCounter_(orderItemVec, itemIdVec))
    {
        return true;
    }

    LOG(ERROR) << "failed in saveOrder_(), USERID: " << userIdStr
               << ", order id: " << orderIdStr
               << ", item num: " << itemIdVec.size();
    return false;
}

bool RecommendTaskService::insertPurchaseCounter_(
    const OrderItemVec& orderItemVec,
    const std::vector<itemid_t>& itemIdVec
)
{
    bool result = true;

    const unsigned int itemNum = orderItemVec.size();
    for (unsigned int i = 0; i < itemNum; ++i)
    {
        const std::string& query = orderItemVec[i].query_;

        if (query.empty())
            continue;

        PurchaseCounter purchaseCounter;
        if (! queryPurchaseCounter_.get(query, purchaseCounter))
        {
            result = false;
            continue;
        }

        purchaseCounter.click(itemIdVec[i]);

        if (! queryPurchaseCounter_.update(query, purchaseCounter))
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
        if (! itemIdGenerator_.strIdToItemId(it->itemIdStr_, itemId))
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

void RecommendTaskService::cronJob_(int calltype)
{
    if (cronExpression_.matches_now() || calltype > 0)
    {
        if(calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(cronJobName_, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << cronJobName_;
            }
            else
                LOG(INFO) << "cron job ignored on replica: " << cronJobName_;
            return;
        }
        DISTRIBUTE_WRITE_BEGIN;
        DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

        boost::mutex::scoped_try_lock lock(buildCollectionMutex_);

        if (lock.owns_lock() == false)
        {
            LOG(INFO) << "exit recommend cron job as still in building collection " << bundleConfig_.collectionName_;
            return;
        }
        CronJobReqLog reqlog;
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName_ << std::endl;
            return;
        }

        bool result = true;
        if (updateRecommendBase_.needRebuildPurchaseSimMatrix())
        {
            updateRecommendBase_.buildPurchaseSimMatrix(result);
        }
        flush();

        buildFreqItemSet_();
        DISTRIBUTE_WRITE_FINISH(true);
    }
}

void RecommendTaskService::flush()
{
    LOG(INFO) << "start flushing recommend data for collection " << bundleConfig_.collectionName_;

    userManager_.flush();
    visitManager_.flush();
    purchaseManager_.flush();
    cartManager_.flush();
    orderManager_.flush();
    eventManager_.flush();
    rateManager_.flush();

    queryPurchaseCounter_.flush();

    bool result = true;
    updateRecommendBase_.flushRecommendMatrix(result);

    LOG(INFO) << "finish flushing recommend data for collection " << bundleConfig_.collectionName_;
}

} // namespace sf1r

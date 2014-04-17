#include <configuration-manager/PropertyConfig.h>
#include <document-manager/DocumentManager.h>
#include <common/PropSharedLockSet.h>
#include <common/QueryNormalizer.h>
#include "MiningManager.h"
#include "MiningTaskBuilder.h"
#include "MultiThreadMiningTaskBuilder.h"
#include "MiningTask.h"

#include "group-manager/GroupManager.h"
#include "group-manager/GroupFilterBuilder.h"
#include "group-manager/GroupFilter.h"
#include "attr-manager/AttrManager.h"

#include "util/convert_ustr.h"
#include "util/FSUtil.hpp"
#include "group-label-logger/GroupLabelLogger.h"
#include "group-label-logger/GroupLabelKnowledge.h"
#include "merchant-score-manager/MerchantScoreManager.h"
#include "custom-rank-manager/CustomDocIdConverter.h"
#include "custom-rank-manager/CustomRankManager.h"
#include "product-scorer/ProductScorerFactory.h"
#include "product-score-manager/ProductScoreManager.h"
#include "product-score-manager/OfflineProductScorerFactoryImpl.h"
#include "product-score-manager/ProductScoreTable.h"
#include "product-ranker/ProductRankerFactory.h"
#include "product-forward/ProductForwardManager.h"
#include "product-forward/ProductForwardMiningTask.h"


#include "suffix-match-manager/SuffixMatchManager.hpp"
#include "product-tokenizer/ProductTokenizerFactory.h"
#include "product-tokenizer/ProductTokenizer.h"
#include "product-tokenizer/FuzzyNormalizerFactory.h"
#include "product-tokenizer/FuzzyNormalizer.h"
#include "suffix-match-manager/FilterManager.h"
#include "suffix-match-manager/FMIndexManager.h"

#include <index-manager/zambezi-manager/ZambeziManager.h>

#include "ad-index-manager/AdIndexManager.h"

#include <search-manager/SearchManager.h>
#include <search-manager/NumericPropertyTableBuilderImpl.h>
#include <search-manager/RTypeStringPropTableBuilder.h>
#include <index-manager/InvertedIndexManager.h>
#include <common/SearchCache.h>
#include <common/ResourceManager.h>

#include <directory-manager/DirectoryCookie.h>


#include <ir/index_manager/index/IndexReader.h>
#include <ir/id_manager/IDManager.h>
#include <la-manager/LAManager.h>
#include <la-manager/KNlpDictMonitor.h>
#include <la-manager/AttrTokenizeWrapper.h>

#include <am/3rdparty/rde_hash.h>
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <util/driver/Request.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/timer.hpp>
#include <boost/scoped_ptr.hpp>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>

#include <fstream>
#include <iterator>
#include <set>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <algorithm>
#include <memory> // auto_ptr

namespace
{
const izenelib::util::UString::EncodingType kEncodeType =
    izenelib::util::UString::UTF_8;

const std::string kTopLabelPropName = "Category";
const size_t kTopLabelDocNum = 1000;
const size_t kTopLabelCateNum = 4;

double elapsedFromLast(izenelib::util::ClockTimer& clock, double& lastSec)
{
    double curSec = clock.elapsed();
    double result = curSec - lastSec;
    lastSec = curSec;
    return result;
}

typedef std::pair<double, uint32_t> ScoreDocId;
const size_t kTopAverageNum = 10;

double topAverageScore(const std::vector<ScoreDocId>& resultList)
{
    size_t num = std::min(kTopAverageNum, resultList.size());
    if (num == 0)
        return 0;

    double sum = 0;
    for (size_t i = 0; i < num; ++i)
    {
        sum += resultList[i].first;
    }
    return sum / num;
}

const uint32_t kFuzzySearchMinLucky = 500;

}

namespace sf1r
{

bool greater_pair(const std::pair<std::string, double>& obj1, const std::pair<std::string, double>& obj2)
{
    return obj1.second > obj2.second;
}

using namespace boost::filesystem;
using namespace izenelib::ir::idmanager;
using namespace sf1r::b5m;
namespace bfs = boost::filesystem;

std::string MiningManager::system_resource_path_;
std::string MiningManager::system_working_path_;

MiningManager::MiningManager(
        const std::string& collectionDataPath,
        const CollectionPath& collectionPath,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<LAManager>& laManager,
        const boost::shared_ptr<InvertedIndexManager>& index_manager,
        const boost::shared_ptr<SearchManager>& searchManager,
        const boost::shared_ptr<SearchCache>& searchCache,
        const boost::shared_ptr<IDManager>& idManager,
        const std::string& collectionName,
        const DocumentSchema& documentSchema,
        const MiningConfig& miningConfig,
        const MiningSchema& miningSchema,
        const IndexBundleSchema& indexSchema)
    : collectionDataPath_(collectionDataPath)
    , queryDataPath_(collectionPath.getQueryDataPath())
    , collectionPath_(collectionPath)
    , collectionName_(collectionName)
    , documentSchema_(documentSchema)
    , miningConfig_(miningConfig)
    , indexSchema_ (indexSchema)
    , mining_schema_(miningSchema)
    , document_manager_(documentManager)
    , laManager_(laManager)
    , index_manager_(index_manager)
    , searchManager_(searchManager)
    , searchCache_(searchCache)
    , idManager_(idManager)
    , numericTableBuilder_(NULL)
    , rtypeStringPropTableBuilder_(NULL)
    , groupManager_(NULL)
    , attrManager_(NULL)
    , groupFilterBuilder_(NULL)
    , merchantScoreManager_(NULL)
    , customDocIdConverter_(NULL)
    , customRankManager_(NULL)
    , offlineScorerFactory_(NULL)
    , productScoreManager_(NULL)
    , productForwardManager_(NULL)
    , groupLabelKnowledge_(NULL)
    , productScorerFactory_(NULL)
    , productRankerFactory_(NULL)
    , suffixMatchManager_(NULL)
    , productTokenizer_(NULL)
    , adIndexManager_(NULL)
    , miningTaskBuilder_(NULL)
    , multiThreadMiningTaskBuilder_(NULL)
    , hasDeletedDocDuringMining_(false)
{
}

MiningManager::~MiningManager()
{
    if (adIndexManager_) delete adIndexManager_;
    if (multiThreadMiningTaskBuilder_) delete multiThreadMiningTaskBuilder_;
    if (miningTaskBuilder_) delete miningTaskBuilder_;
    if (productRankerFactory_) delete productRankerFactory_;
    if (productScorerFactory_) delete productScorerFactory_;
    if (groupLabelKnowledge_) delete groupLabelKnowledge_;
    if (productForwardManager_) delete productForwardManager_;
    if (productScoreManager_) delete productScoreManager_;
    if (offlineScorerFactory_) delete offlineScorerFactory_;
    if (customRankManager_) delete customRankManager_;
    if (customDocIdConverter_) delete customDocIdConverter_;
    if (merchantScoreManager_) delete merchantScoreManager_;
    if (groupFilterBuilder_) delete groupFilterBuilder_;
    if (groupManager_) delete groupManager_;
    if (attrManager_) delete attrManager_;
    if (numericTableBuilder_) delete numericTableBuilder_;
    if (rtypeStringPropTableBuilder_) delete rtypeStringPropTableBuilder_;
    if (suffixMatchManager_) delete suffixMatchManager_;
    if (productTokenizer_) delete productTokenizer_;

    close();
}

bool MiningManager::startSynonym_ = 0;

void MiningManager::close()
{
    for (GroupLabelLoggerMap::iterator it = groupLabelLoggerMap_.begin();
            it != groupLabelLoggerMap_.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
    groupLabelLoggerMap_.clear();
}

bool MiningManager::open()
{
    close();

    std::cout << "DO_GROUP : " << (int) mining_schema_.group_enable << std::endl;
    std::cout << "DO_ATTR : " << (int) mining_schema_.attr_enable << std::endl;

    /** Global variables **/
    try
    {
        bfs::path cookiePath(bfs::path(basicPath_) / "cookie");
        directory::DirectoryCookie cookie(cookiePath);
        if (cookie.read())
        {
            if (!cookie.valid())
            {
                LOG(INFO) << "Mining data is broken.";
                boost::filesystem::remove_all(basicPath_);
                LOG(INFO) << "Mining data deleted.";
            }
        }

        std::string prefix_path  = collectionDataPath_;
        FSUtil::createDir(prefix_path);

        /** analyzer */

        std::string kma_path;
        LAPool::getInstance()->get_kma_path(kma_path);
        std::string cma_path;
        LAPool::getInstance()->get_cma_path(cma_path);
        std::string jma_path;
        LAPool::getInstance()->get_jma_path(jma_path);

        /**Miningtask Builder*/
        if (mining_schema_.suffixmatch_schema.suffix_match_enable ||
            mining_schema_.group_enable ||
            mining_schema_.attr_enable ||
            mining_schema_.product_ranking_config.isEnable ||
            mining_schema_.zambezi_config.isEnable ||
            mining_schema_.ad_index_config.isEnable)
        {
            miningTaskBuilder_ = new MiningTaskBuilder( document_manager_);
            multiThreadMiningTaskBuilder_ = new MultiThreadMiningTaskBuilder(
                document_manager_, miningConfig_.mining_task_param.threadNum);
        }

        if (rtypeStringPropTableBuilder_) delete rtypeStringPropTableBuilder_;
        rtypeStringPropTableBuilder_ = new RTypeStringPropTableBuilder(*document_manager_);

        /** group */
        if (mining_schema_.group_enable)
        {
            if (groupManager_) delete groupManager_;
            std::string groupPath = prefix_path + "/group";

            groupManager_ = new faceted::GroupManager(
                    mining_schema_.group_config_map,
                    *document_manager_, groupPath);
            if (! groupManager_->open())
            {
                std::cerr << "open GROUP failed" << std::endl;
                return false;
            }
            std::vector<MiningTask*>& miningTaskList = groupManager_->getGroupMiningTask();
            for (std::vector<MiningTask*>::const_iterator i = miningTaskList.begin(); i != miningTaskList.end(); ++i)
            {
                miningTaskBuilder_->addTask(*i);
            }
            miningTaskList.clear();
        }

        /** attr */
        if (mining_schema_.attr_enable)
        {
            if (attrManager_) delete attrManager_;
            std::string attrPath = prefix_path + "/attr";

            attrManager_ = new faceted::AttrManager(mining_schema_.attr_property, attrPath, *document_manager_.get());
            if (! attrManager_->open())
            {
                std::cerr << "open ATTR failed" << std::endl;
                return false;
            }
            MiningTask* miningTask = attrManager_->getAttrMiningTask();
            miningTaskBuilder_->addTask(miningTask);
        }

        if (groupManager_ || attrManager_)
        {
            if (groupFilterBuilder_) delete groupFilterBuilder_;

            groupFilterBuilder_ = new faceted::GroupFilterBuilder(
                mining_schema_.group_config_map,
                groupManager_,
                attrManager_,
                numericTableBuilder_);
        }

        /** group label log */
        if (mining_schema_.group_enable)
        {
            std::string logPath = prefix_path + "/group_label_log";
            try
            {
                FSUtil::createDir(logPath);
            }
            catch (FileOperationException& e)
            {
                LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
                return false;
            }

            for (GroupConfigMap::const_iterator it = mining_schema_.group_config_map.begin();
                    it != mining_schema_.group_config_map.end(); ++it)
            {
                if (! it->second.isStringType())
                    continue;

                const std::string& propName = it->first;
                if (!groupLabelLoggerMap_[propName])
                {
                    std::auto_ptr<GroupLabelLogger> loggerPtr(new GroupLabelLogger(logPath, propName));
                    groupLabelLoggerMap_[propName] = loggerPtr.release();
                }
                else
                {
                    std::cerr << "the label logger on group property " << propName
                        << " is already opened before" << std::endl;
                    return false;
                }
            }
        }

        if (customDocIdConverter_) delete customDocIdConverter_;
        customDocIdConverter_ = new CustomDocIdConverter(*idManager_);

        /** Suffix Match */
        if (mining_schema_.suffixmatch_schema.suffix_match_enable)
        {
            LOG(INFO) << "suffix match enabled.";

            ProductTokenizerFactory tokenizerFactory(system_resource_path_);
            productTokenizer_ = tokenizerFactory.createProductTokenizer(
                mining_schema_.suffixmatch_schema.suffix_match_tokenize_dicpath);

            if (!productTokenizer_)
            {
                LOG(ERROR) << "failed to create ProductTokenizer.";
                return false;
            }

            if (!KNlpResourceManager::getResource()->loadDictFiles())
                return false;

            KNlpDictMonitor::get()->start(system_resource_path_ + "/dict/term_category");

            FuzzyNormalizerFactory normalizerFactory(productTokenizer_);
            FuzzyNormalizer* fuzzyNormalizer =
                    normalizerFactory.createFuzzyNormalizer(
                        mining_schema_.suffixmatch_schema.normalizer_config);

            suffix_match_path_ = prefix_path + "/suffix_match";
            suffixMatchManager_ = new SuffixMatchManager(
                suffix_match_path_, document_manager_,
                groupManager_, attrManager_, numericTableBuilder_,
                fuzzyNormalizer);
            suffixMatchManager_->addFMIndexProperties(mining_schema_.suffixmatch_schema.searchable_properties, FMIndexManager::LESS_DV);
            suffixMatchManager_->addFMIndexProperties(mining_schema_.suffixmatch_schema.suffix_match_properties, FMIndexManager::COMMON, true);

            // reading suffix config and load filter data here.
            boost::shared_ptr<FilterManager>& filter_manager = suffixMatchManager_->getFilterManager();
            filter_manager->setGroupFilterProperties(mining_schema_.suffixmatch_schema.group_filter_properties);
            filter_manager->setAttrFilterProperties(mining_schema_.suffixmatch_schema.attr_filter_properties);
            filter_manager->setStrFilterProperties(mining_schema_.suffixmatch_schema.str_filter_properties);
            filter_manager->setDateFilterProperties(mining_schema_.suffixmatch_schema.date_filter_properties);
            const std::vector<NumericFilterConfig>& number_config_list = mining_schema_.suffixmatch_schema.num_filter_properties;
            std::vector<std::string> number_props;
            number_props.reserve(number_config_list.size());
            std::vector<int32_t> number_amp_list;
            number_amp_list.reserve(number_config_list.size());
            for (size_t i = 0; i < number_config_list.size(); ++i)
            {
                number_props.push_back(number_config_list[i].property);
                number_amp_list.push_back(number_config_list[i].amplifier);
            }
            filter_manager->setNumFilterProperties(number_props, number_amp_list);
            filter_manager->loadFilterId();

            if (mining_schema_.suffixmatch_schema.suffix_match_enable)
            {
                suffixMatchManager_->buildMiningTask();
                MiningTask* miningTask = suffixMatchManager_->getMiningTask();
                miningTaskBuilder_->addTask(miningTask);
            }

            if (mining_schema_.suffixmatch_schema.product_forward_enable)
            {
                initProductForwardManager_();
            }
        }

        if(!initAdIndexManager_(mining_schema_.ad_index_config))
        {
            LOG(ERROR) << "init AdIndexManager fail"<<endl;
            return false;
        }
        /** product rank */
        const ProductRankingConfig& rankConfig =
            mining_schema_.product_ranking_config;

        if (!initMerchantScoreManager_(rankConfig) ||
            !initGroupLabelKnowledge_(rankConfig) ||
            !initProductScorerFactory_(rankConfig) ||
            !initProductRankerFactory_(rankConfig))
            return false;
    }
    catch (NotEnoughMemoryException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (MiningConfigException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (FileOperationException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (ResourceNotFoundException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (FileCorruptedException& ex)
    {
        LOG(ERROR) << ex.toString();
        return false;
    }
    catch (boost::filesystem::filesystem_error& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    catch (izenelib::ir::indexmanager::IndexManagerException& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << ex.what();
        return false;
    }
    return true;
}

bool MiningManager::DoMiningCollectionFromAPI()
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    TimestampReqLog reqlog;
    reqlog.timestamp = Utilities::createTimeStamp();
    if (!DistributeRequestHooker::get()->prepare(Req_WithTimestamp, reqlog))
    {
        LOG(INFO) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = false;
    try
    {
        ret = DoMiningCollection(reqlog.timestamp);
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        ret = false;
    }

    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
    return ret;
}

void MiningManager::DoContinue()
{
    //do mining continue;
    if (NodeManagerBase::get()->isDistributed())
    {
        LOG(INFO) << "continue not allowed in distributed sf1r.";
        return;
    }
    try
    {
        std::string continue_file = collectionDataPath_+"/continue";
        if (boost::filesystem::exists(continue_file))
        {
            boost::filesystem::remove_all(continue_file);
            DoMiningCollection(0);
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

bool MiningManager::DOMiningTask(int64_t timestamp)
{

    if (multiThreadMiningTaskBuilder_)
    {
        multiThreadMiningTaskBuilder_->buildCollection(timestamp);
    }

    if (miningTaskBuilder_)
    {
        miningTaskBuilder_->buildCollection(timestamp);
    }
    return true;
}

bool MiningManager::DoMiningCollection(int64_t timestamp)
{
    // calculate product score
    if (mining_schema_.product_ranking_config.isEnable)
    {
        productScoreManager_->buildCollection();
    }

    DOMiningTask(timestamp);

    hasDeletedDocDuringMining_ = false;
    LOG (INFO) << "Clear Rtype Docid List";
    document_manager_->RtypeDocidPros_.clear();
    document_manager_->last_delete_docid_.clear();
    return true;
}

void MiningManager::onIndexUpdated(size_t docNum)
{
}

bool MiningManager::visitDoc(uint32_t docId)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = false;//ctrManager_->update(docId);

    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::clickGroupLabel(
    const std::string& query,
    const std::string& propName,
    const std::vector<std::string>& groupPath
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    faceted::PropValueTable::pvid_t pvId = propValueId_(propName, groupPath);
    if (pvId == 0)
    {
        return false;
    }

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = logger->logLabel(query, pvId);
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::getFreqGroupLabel(
    const std::string& query,
    const std::string& propName,
    int limit,
    std::vector<std::vector<std::string> >& pathVec,
    std::vector<int>& freqVec
)
{
    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    if (! logger->getFreqLabel(query, limit, pvIdVec, freqVec))
    {
        LOG(ERROR) << "error in GroupLabelLogger::getFreqLabel(), query: " << query;
        return false;
    }

    return propValuePaths_(propName, pvIdVec, pathVec, true);
}

bool MiningManager::propValuePaths_(
    const std::string& propName,
    const std::vector<faceted::PropValueTable::pvid_t>& pvIds,
    std::vector<std::vector<std::string> >& paths,
    bool isLock)
{
    if (! groupManager_)
    {
        LOG(ERROR) << "the GroupManager is not initialized";
        return false;
    }

    const faceted::PropValueTable* propValueTable = groupManager_->getPropValueTable(propName);
    if (! propValueTable)
    {
        LOG(ERROR) << "the PropValueTable is not initialized for group property: " << propName;
        return false;
    }

    for (std::vector<faceted::PropValueTable::pvid_t>::const_iterator idIt = pvIds.begin();
            idIt != pvIds.end(); ++idIt)
    {
        std::vector<izenelib::util::UString> ustrPath;
        propValueTable->propValuePath(*idIt, ustrPath, isLock);

        std::vector<std::string> path;
        for (std::vector<izenelib::util::UString>::const_iterator ustrIt = ustrPath.begin();
             ustrIt != ustrPath.end(); ++ustrIt)
        {
            std::string str;
            ustrIt->convertString(str, UString::UTF_8);
            path.push_back(str);
        }

        paths.push_back(path);
    }

    return true;
}

faceted::PropValueTable::pvid_t MiningManager::propValueId_(
        const std::string& propName,
        const std::vector<std::string>& groupPath
) const
{
    faceted::PropValueTable::pvid_t pvId = 0;
    const faceted::PropValueTable* propValueTable = NULL;

    if (groupManager_ &&
        (propValueTable = groupManager_->getPropValueTable(propName)))
    {
        std::vector<izenelib::util::UString> ustrPath;
        convert_to_ustr_vector(groupPath, ustrPath);
        pvId = propValueTable->propValueId(ustrPath);

        if (pvId == 0)
        {
            std::string pathStr;
            for (std::vector<std::string>::const_iterator it = groupPath.begin();
                    it != groupPath.end(); ++it)
            {
                pathStr += *it;
                pathStr += '>';
            }
            LOG(WARNING) << "no pvid_t exists for group label path: " << pathStr;
        }
    }
    else
    {
        LOG(WARNING) << "PropValueTable is not initialized for group property: " << propName;
    }

    return pvId;
}

bool MiningManager::setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::vector<std::string> >& groupLabels
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    GroupLabelLogger* logger = groupLabelLoggerMap_[propName];
    if (! logger)
    {
        LOG(ERROR) << "GroupLabelLogger is not initialized for group property: " << propName;
        return false;
    }

    std::vector<faceted::PropValueTable::pvid_t> pvIdVec;
    for (std::vector<std::vector<std::string> >::const_iterator it = groupLabels.begin();
        it != groupLabels.end(); ++it)
    {
        faceted::PropValueTable::pvid_t pvId = propValueId_(propName, *it);
        if (pvId == 0)
        {
            return false;
        }

        pvIdVec.push_back(pvId);
    }
    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = logger->setTopLabel(query, pvIdVec);
    DISTRIBUTE_WRITE_FINISH(ret);
    return ret;
}

bool MiningManager::getMerchantScore(
    const std::vector<std::string>& merchantNames,
    MerchantStrScoreMap& merchantScoreMap
) const
{
    if (! merchantScoreManager_)
        return false;

    if (merchantNames.empty())
    {
        merchantScoreManager_->getAllStrScore(merchantScoreMap);
    }
    else
    {
        merchantScoreManager_->getStrScore(merchantNames, merchantScoreMap);
    }

    return true;
}

bool MiningManager::setMerchantScore(const MerchantStrScoreMap& merchantScoreMap)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    if (! merchantScoreManager_)
    {
        return false;
    }
    NoAdditionReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    merchantScoreManager_->setScore(merchantScoreMap);

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

bool MiningManager::setCustomRank(
    const std::string& query,
    const CustomRankDocStr& customDocStr
)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    //CustomRankDocId customDocId;

    //bool convertResult = customDocIdConverter_ &&
    //    customDocIdConverter_->convert(customDocStr, customDocId);

    //if (!convertResult)
    //{
    //    return false;
    //}

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool setResult = customRankManager_ &&
        customRankManager_->setCustomValue(query, customDocStr);

    DISTRIBUTE_WRITE_FINISH(/*convertResult &&*/ setResult);

    return /*convertResult &&*/ setResult;
}

bool MiningManager::getCustomRank(
    const std::string& query,
    std::vector<Document>& topDocList,
    std::vector<Document>& excludeDocList
)
{
    CustomRankDocId customDocId;

    return customRankManager_ &&
           customRankManager_->getCustomValue(query, customDocId) &&
           getDocList_(customDocId.topIds, topDocList) &&
           getDocList_(customDocId.excludeIds, excludeDocList);
}

bool MiningManager::getCustomQueries(std::vector<std::string>& queries)
{
    return customRankManager_ &&
           customRankManager_->getQueries(queries);
}

bool MiningManager::getDocList_(
    const std::vector<docid_t>& docIdList,
    std::vector<Document>& docList
)
{
    Document doc;

    for (std::vector<docid_t>::const_iterator it = docIdList.begin();
        it != docIdList.end(); ++it)
    {
        docid_t docId = *it;

        if (document_manager_->getDocument(docId, doc))
        {
            document_manager_->getRTypePropertiesForDocument(docId, doc);
            docList.push_back(doc);
        }
        else
        {
            LOG(WARNING) << "in getDocList_(), docid_t " << docId
                         << " does not exist";
        }
    }

    return true;
}


namespace
{
struct IsDeleted
{
    IsDeleted(const boost::shared_ptr<DocumentManager>& doc_manager)
        :inner_doc_manager_(doc_manager)
    {
    }
    bool operator()(const std::pair<double, uint32_t>& single_res) const
    {
        return inner_doc_manager_->isDeleted(single_res.second);
    }
private:
    const boost::shared_ptr<DocumentManager>& inner_doc_manager_;
};
}

bool MiningManager::GetSuffixMatch(
        const SearchKeywordOperation& actionOperation,
        uint32_t max_docs,
        bool use_fuzzy,
        uint32_t start,
        const std::vector<QueryFiltering::FilteringType>& filter_param,
        std::vector<uint32_t>& docIdList,
        std::vector<float>& rankScoreList,
        std::vector<float>& customRankScoreList,
        std::size_t& totalCount,
        faceted::GroupRep& groupRep,
        sf1r::faceted::OntologyRep& attrRep,
        bool isAnalyzeQuery,
        UString& analyzedQuery,
        std::string& pruneQueryString_,
        DistKeywordSearchInfo& distSearchInfo,
        faceted::GroupParam::GroupLabelScoreMap& topLabelMap)
{
    if (!mining_schema_.suffixmatch_schema.suffix_match_enable || !suffixMatchManager_)
        return false;

    izenelib::util::ClockTimer clock;
    double lastSec, tokenTime, suffixMatchTime, productScoreTime, groupTime;
    lastSec = tokenTime = suffixMatchTime = productScoreTime = groupTime = 0;

    izenelib::util::UString queryU(actionOperation.actionItem_.env_.queryString_, izenelib::util::UString::UTF_8);
    std::vector<std::pair<double, uint32_t> > res_list;

    const std::vector<string>& search_in_properties = actionOperation.actionItem_.searchPropertyList_;

    size_t orig_max_docs = max_docs;

    if (!use_fuzzy)
    {
        totalCount = suffixMatchManager_->longestSuffixMatch(
                actionOperation.actionItem_.env_.queryString_,
                search_in_properties,
                max_docs,
                res_list);
    }
    else
    {
        if (actionOperation.actionItem_.groupParam_.groupProps_.size() > 0 ||
            actionOperation.actionItem_.groupParam_.isAttrGroup_)
        {
            // need do counter.
            max_docs = mining_schema_.suffixmatch_schema.suffix_groupcounter_topk;
            LOG(INFO) << "topk doc num is :" << max_docs;
        }

        LOG(INFO) << "suffix searching using fuzzy mode " << endl;

        std::string pattern_orig = actionOperation.actionItem_.env_.queryString_;

        if (pattern_orig.empty())
            return 0;

        LOG(INFO) << "original query string: " << pattern_orig;
        std::string pattern = pattern_orig;
        boost::to_lower(pattern);

        const bool isWrongQuery = QueryNormalizer::get()->isWrongQuery(pattern);
        if (isWrongQuery)
        {
            LOG (ERROR) <<"The query is too long...";
        }

        const bool isLongQuery = QueryNormalizer::get()->isLongQuery(pattern);
        boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
        if (isLongQuery)
            pattern = knlpWrapper->cleanStopword(pattern);
        else
            pattern = knlpWrapper->cleanGarbage(pattern);

        LOG(INFO) << "clear stop word for long query: " << pattern;

        ProductTokenParam tokenParam(pattern, isAnalyzeQuery);

        // use Fuzzy Search Threshold 
        if (actionOperation.actionItem_.searchingMode_.useFuzzyThreshold_)
        {
            tokenParam.useFuzzyThreshold = true;
            tokenParam.fuzzyThreshold = actionOperation.actionItem_.searchingMode_.fuzzyThreshold_;
            tokenParam.tokensThreshold = actionOperation.actionItem_.searchingMode_.tokensThreshold_;
        }

        // use Privilege Query
        if (actionOperation.actionItem_.searchingMode_.usePivilegeQuery_)
        {
            tokenParam.usePrivilegeQuery = true;
            tokenParam.privilegeQuery = actionOperation.actionItem_.searchingMode_.privilegeQuery_;
            tokenParam.privilegeWeight = actionOperation.actionItem_.searchingMode_.privilegeWeight_;
        }

        productTokenizer_->tokenize(tokenParam);
        analyzedQuery.swap(tokenParam.refinedResult);

        double queryScore = productTokenizer_->sumQueryScore(pattern_orig);
        actionOperation.actionItem_.queryScore_ = queryScore;
        std::cout << "The query's product score is:" << queryScore << std::endl;

        for (ProductTokenParam::TokenScoreListIter it = tokenParam.majorTokens.begin();
             it != tokenParam.majorTokens.end(); ++it)
        {
            std::string key;
            it->first.convertString(key, izenelib::util::UString::UTF_8);
            cout << key << " " << it->second << endl;
        }
        std::cout << "-----" << std::endl;
        for (ProductTokenParam::TokenScoreListIter it = tokenParam.minorTokens.begin();
             it != tokenParam.minorTokens.end(); ++it)
        {
            std::string key;
            it->first.convertString(key, izenelib::util::UString::UTF_8);
            cout << key << " " << it->second << endl;
        }

        if (actionOperation.actionItem_.searchingMode_.useQueryPrune_ == false)
        {
            tokenParam.rankBoundary = 0;
        }

        LOG(INFO) << " Rank boundary: " << tokenParam.rankBoundary;

        bool useSynonym = actionOperation.actionItem_.languageAnalyzerInfo_.synonymExtension_;
        bool isAndSearch = true;
        bool isOrSearch = false;

        if (isLongQuery)
        {
            useSynonym = false;
            isAndSearch = false;
            isOrSearch = true;
        }

        //check if is b5mp or b5mo;
        bool isCompare = false;
        const std::string& itemcount = getOfferItemCountPropName_();
        if (!itemcount.empty())
            isCompare = true;

        tokenTime = elapsedFromLast(clock, lastSec);

        LOG(INFO) << "use compare: " << isCompare;
        if (isAndSearch)
        {
            LOG(INFO) << "for short query, try AND search first";
            std::list<std::pair<UString, double> > short_query_major_tokens(tokenParam.majorTokens);
            std::list<std::pair<UString, double> > short_query_minor_tokens;

            for (std::list<std::pair<UString, double> >::iterator it = tokenParam.minorTokens.begin();
                 it != tokenParam.minorTokens.end(); ++it)
            {
                short_query_major_tokens.push_back(*it);
            }

            totalCount = suffixMatchManager_->AllPossibleSuffixMatch(
                    useSynonym,
                    short_query_major_tokens,
                    short_query_minor_tokens,
                    search_in_properties,
                    max_docs,
                    actionOperation.actionItem_.searchingMode_.filtermode_,
                    filter_param,
                    actionOperation.actionItem_.groupParam_,
                    res_list,
                    tokenParam.rankBoundary);

            isOrSearch = res_list.empty();
        }

        if (isOrSearch)
        {
            LOG(INFO) << "for long query or short AND result is empty, "
                      << "try OR search, lucky: " << max_docs;
            totalCount = suffixMatchManager_->AllPossibleSuffixMatch(
                useSynonym,
                tokenParam.majorTokens,
                tokenParam.minorTokens,
                search_in_properties,
                max_docs,
                actionOperation.actionItem_.searchingMode_.filtermode_,
                filter_param,
                actionOperation.actionItem_.groupParam_,
                res_list,
                tokenParam.rankBoundary);

            // while (res_list.empty() && !tokenParam.majorTokens.empty())
            // {
            //     const ProductTokenParam::TokenScore& tokenScore(
            //         tokenParam.majorTokens.back());
            //     std::string token;
            //     tokenScore.first.convertString(token, kEncodeType);

            //     max_docs /= 2;
            //     max_docs = std::max(max_docs, kFuzzySearchMinLucky);

            //     LOG(INFO) << "try OR search again after removing one major token: "
            //               << token << ", lucky: " << max_docs;

            //     tokenParam.minorTokens.push_back(tokenScore);
            //     tokenParam.majorTokens.pop_back();

            //     totalCount = suffixMatchManager_->AllPossibleSuffixMatch(
            //         useSynonym,
            //         tokenParam.majorTokens,
            //         tokenParam.minorTokens,
            //         search_in_properties,
            //         max_docs,
            //         actionOperation.actionItem_.searchingMode_.filtermode_,
            //         filter_param,
            //         actionOperation.actionItem_.groupParam_,
            //         res_list,
            //         tokenParam.rankBoundary);
            // }
        }

        distSearchInfo.majorTokenNum_ = tokenParam.majorTokens.size();

        suffixMatchTime = elapsedFromLast(clock, lastSec);

        if (!res_list.empty())
        {
            LOG(INFO) << "average score of fuzzy search top results: "
                      << topAverageScore(res_list);
        }

        searchManager_->fuzzySearchRanker_.rankByProductScore(
            actionOperation.actionItem_, res_list, isCompare);

        if (mining_schema_.suffixmatch_schema.product_forward_enable)
        {
            std::vector<std::pair<double, uint32_t> > final_res;
            productForwardManager_->forwardSearch(pattern_orig, res_list, final_res);
            LOG(INFO)<<"suffix res num = "<<res_list.size()<<" forward res = "<<final_res.size();
            final_res.swap(res_list);
            totalCount = res_list.size();
        }

        productScoreTime = elapsedFromLast(clock, lastSec);
        if (!mining_schema_.suffixmatch_schema.product_forward_enable)
        {
            getGroupAttrRep_(res_list, actionOperation.actionItem_.groupParam_,
                         groupRep, attrRep,
                         kTopLabelPropName, topLabelMap);
        }

        groupTime = elapsedFromLast(clock, lastSec);
    }

    LOG(INFO) << "GetSuffixMatch(): " << clock.elapsed()
              << ", token: " << tokenTime
              << ", suffix: " << suffixMatchTime
              << ", product score: " << productScoreTime
              << ", group: " << groupTime
              << ", res_list.size(): " << res_list.size()
              << ", query: " << actionOperation.actionItem_.env_.queryString_;

    if (!totalCount ||res_list.empty()) return false;

    //We do not use this post delete filtering because deleted documents should never be searched from
    //suffix index in normal cases, while if there are deleted documents before mining finished, these documents
    //should be abled to searched as well.
    //We ignore the exception case that SF1 quit before mining finished
    //if (!hasDeletedDocDuringMining_)
    //    res_list.erase(std::remove_if (res_list.begin(), res_list.end(), IsDeleted(document_manager_)), res_list.end());
    res_list.resize(std::min(orig_max_docs, res_list.size()));

    docIdList.resize(res_list.size());
    rankScoreList.resize(res_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        rankScoreList[i] = res_list[i].first;
        docIdList[i] = res_list[i].second;
    }
    if (!mining_schema_.suffixmatch_schema.product_forward_enable)
    {
        searchManager_->fuzzySearchRanker_.rankByPropValue(
            actionOperation, start, docIdList, rankScoreList, customRankScoreList, distSearchInfo);
    }

    cout<<"return true"<<endl;

    return true;
}

void MiningManager::getGroupAttrRep_(
    const std::vector<std::pair<double, uint32_t> >& res_list,
    faceted::GroupParam& groupParam,
    faceted::GroupRep& groupRep,
    sf1r::faceted::OntologyRep& attrRep,
    const std::string& topPropName,
    faceted::GroupParam::GroupLabelScoreMap& topLabelMap)
{
    if (!groupFilterBuilder_ || (!groupManager_ && !attrManager_))
        return;

    PropSharedLockSet propSharedLockSet;
    boost::scoped_ptr<faceted::GroupFilter> groupFilter(
        groupFilterBuilder_->createFilter(groupParam, propSharedLockSet));
    if (!groupFilter)
        return;

    const faceted::PropValueTable* categoryValueTable = GetPropValueTable(topPropName);
    if (!categoryValueTable)
        return;

    propSharedLockSet.insertSharedLock(categoryValueTable);

    std::vector<faceted::PropValueTable::pvid_t> topCateIds;
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        if (topCateIds.size() < kTopLabelCateNum && i < kTopLabelDocNum)
        {
            category_id_t catId =
                categoryValueTable->getFirstValueId(res_list[i].second);

            if (catId != 0 &&
                std::find(topCateIds.begin(), topCateIds.end(), catId) == topCateIds.end())
            {
                topCateIds.push_back(catId);
            }
        }

        groupFilter->test(res_list[i].second);
    }

    if (topCateIds.empty())
    {
        groupFilter->getGroupRep(groupRep, attrRep);
        return;
    }

    faceted::GroupParam::GroupPathScoreVec& topScoreLabels = topLabelMap[topPropName];
    faceted::GroupParam::GroupPathVec topLabels;
    if (topScoreLabels.empty())
    {
        propValuePaths_(topPropName, topCateIds, topLabels, false);
        topScoreLabels.resize(topLabels.size());
        for(size_t i = 0; i < topLabels.size(); ++i)
        {
            topScoreLabels[i] = std::make_pair(topLabels[i], faceted::GroupPathScoreInfo());
        }
    }

    // get all group results
    sf1r::faceted::OntologyRep tempAttrRep;
    groupFilter->getGroupRep(groupRep, tempAttrRep);

    // replace select label to top label
    if (!topLabels.empty())
    {
        faceted::GroupParam::GroupPathVec& groupLabels =
            groupParam.groupLabels_[topPropName];
        groupLabels.clear();
        groupLabels.push_back(topLabels[0]);
    }

    // get attr results under top label
    boost::scoped_ptr<faceted::GroupFilter> attrGroupFilter(
        groupFilterBuilder_->createFilter(groupParam, propSharedLockSet));

    for (size_t i = 0; i < res_list.size(); ++i)
    {
        attrGroupFilter->test(res_list[i].second);
    }

    faceted::GroupRep tempGroupRep;
    attrGroupFilter->getGroupRep(tempGroupRep, attrRep);
}

const faceted::PropValueTable* MiningManager::GetPropValueTable(const std::string& propName) const
{
    if (! groupManager_)
        return NULL;

    return groupManager_->getPropValueTable(propName);
}

const faceted::AttrTable* MiningManager::GetAttrTable() const
{
    if (! attrManager_)
        return NULL;

    return &attrManager_->getAttrTable();
}

bool MiningManager::getProductScore(
        const std::string& docIdStr,
        const std::string& scoreTypeName,
        score_t& scoreValue)
{
    docid_t docId = 0;

    if (!customDocIdConverter_ ||
        !customDocIdConverter_->convertDocId(docIdStr, docId))
        return false;

    ProductScoreType scoreType =
        mining_schema_.product_ranking_config.getScoreType(scoreTypeName);

    const ProductScoreTable* productScoreTable =
        productScoreManager_->getProductScoreTable(scoreType);

    if (productScoreTable == NULL)
    {
        LOG(WARNING) << "unknown score type: " << scoreTypeName;
        return false;
    }

    scoreValue = productScoreTable->getScoreHasLock(docId);
    return true;
}

bool MiningManager::initMerchantScoreManager_(const ProductRankingConfig& rankConfig)
{
    const ProductScoreConfig& merchantScoreConfig =
        rankConfig.scores[MERCHANT_SCORE];

    if (merchantScoreConfig.propName.empty() || !groupManager_)
        return true;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path scoreDir(parentDir / "merchant_score");
    bfs::create_directories(scoreDir);

    faceted::PropValueTable* merchantValueTable =
        groupManager_->getPropValueTable(merchantScoreConfig.propName);
    const ProductScoreConfig& categoryScoreConfig =
        rankConfig.scores[CATEGORY_SCORE];
    faceted::PropValueTable* categoryValueTable =
        groupManager_->getPropValueTable(categoryScoreConfig.propName);

    if (merchantScoreManager_) delete merchantScoreManager_;

    merchantScoreManager_ = new MerchantScoreManager(
        merchantValueTable, categoryValueTable);

    bfs::path scorePath = scoreDir / "score.bin";
    if (!merchantScoreManager_->open(scorePath.string()))
    {
        LOG(ERROR) << "open " << scorePath << " failed";
        return false;
    }

    return true;
}

bool MiningManager::initGroupLabelKnowledge_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable || !groupManager_)
        return true;

    if (groupLabelKnowledge_) delete groupLabelKnowledge_;

    const ProductScoreConfig& categoryScoreConfig =
        rankConfig.scores[CATEGORY_SCORE];

    const faceted::PropValueTable* categoryValueTable =
        groupManager_->getPropValueTable(categoryScoreConfig.propName);

    const bfs::path resourcePath(system_resource_path_);
    const bfs::path categoryBoostDir(resourcePath / "category-boost");
    if (bfs::exists(categoryBoostDir) && categoryValueTable)
    {
        groupLabelKnowledge_ = new GroupLabelKnowledge(collectionName_,
                                                       *categoryValueTable);

        if (!groupLabelKnowledge_->open(categoryBoostDir.string()))
        {
            LOG(ERROR) << "error in opening " << categoryBoostDir;
        }
    }

    return true;
}

bool MiningManager::initProductForwardManager_()
{
    if (productForwardManager_) delete productForwardManager_;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path forwardDir(parentDir / "forward_index");
    bfs::create_directories(forwardDir);

    productForwardManager_ = new ProductForwardManager(
        forwardDir.string(), "Title");

    if (!productForwardManager_->open())
    {
        LOG(ERROR) << "open " << forwardDir << " failed";
    }

    MiningTask* miningTask_forward =  new ProductForwardMiningTask(
        document_manager_, productForwardManager_);
    miningTaskBuilder_->addTask(miningTask_forward);

    LOG(INFO)<<"product forward init ok";
    return true;
}

bool MiningManager::initProductScorerFactory_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable)
        return true;

    LOG(INFO) << rankConfig.toStr();

    if (productScorerFactory_) delete productScorerFactory_;
    if (productScoreManager_) delete productScoreManager_;
    if (offlineScorerFactory_) delete offlineScorerFactory_;
    if (customRankManager_) delete customRankManager_;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path customRankDir(parentDir / "custom_rank");
    bfs::create_directories(customRankDir);

    const bfs::path customRankPath(customRankDir / "custom.db");
    customRankManager_ = new CustomRankManager(customRankPath.string(),
                                               *customDocIdConverter_,
                                               document_manager_.get());

    offlineScorerFactory_ = new OfflineProductScorerFactoryImpl(
        numericTableBuilder_);

    const bfs::path scoreDir(parentDir / "product_score");
    productScoreManager_ = new ProductScoreManager(
        rankConfig,
        miningConfig_.product_ranking_param,
        *offlineScorerFactory_,
        *document_manager_,
        collectionName_,
        scoreDir.string(),
        searchCache_);

    if (!productScoreManager_->open())
    {
        LOG(ERROR) << "open " << scoreDir << " failed";
        return false;
    }

    productScorerFactory_ = new ProductScorerFactory(rankConfig, *this);
    return true;
}

bool MiningManager::initProductRankerFactory_(const ProductRankingConfig& rankConfig)
{
    if (!rankConfig.isEnable)
        return true;

    const std::string& offerCountPropName =
        rankConfig.scores[OFFER_ITEM_COUNT_SCORE].propName;
    const std::string& diversityPropName =
        rankConfig.scores[DIVERSITY_SCORE].propName;
    const score_t randomScoreWeight =
        rankConfig.scores[RANDOM_SCORE].weight;

    if (offerCountPropName.empty() &&
        diversityPropName.empty() &&
        randomScoreWeight == 0)
        return true;

    const std::string& categoryPropName =
        rankConfig.scores[CATEGORY_SCORE].propName;

    const faceted::PropValueTable* categoryValueTable =
        GetPropValueTable(categoryPropName);

    const boost::shared_ptr<const NumericPropertyTableBase>& offerItemCountTable =
        numericTableBuilder_->createPropertyTable(offerCountPropName);

    if (!offerCountPropName.empty() && !offerItemCountTable)
    {
        LOG(ERROR) << "the NumericPropertyTableBase is not initialized for property: "
                   << offerCountPropName;
        return false;
    }

    const faceted::PropValueTable* diversityValueTable =
        GetPropValueTable(diversityPropName);

    if (!diversityPropName.empty() && !diversityValueTable)
    {
        LOG(ERROR) << "the PropValueTable is not initialized for property: "
                   << diversityPropName;
        return false;
    }

    if (productRankerFactory_) delete productRankerFactory_;
    productRankerFactory_ = new ProductRankerFactory(rankConfig,
                                                     categoryValueTable,
                                                     offerItemCountTable,
                                                     diversityValueTable,
                                                     merchantScoreManager_);
    return true;
}

bool MiningManager::initAdIndexManager_(AdIndexConfig& adIndexConfig)
{
    if (!adIndexConfig.isEnable)
        return true;

    const bfs::path parentDir(collectionDataPath_);
    const bfs::path adIndexDir(parentDir / "adindex");
    bfs::create_directories(adIndexDir);

    if (adIndexManager_) delete adIndexManager_;

    adIndexManager_ = new AdIndexManager(
        system_resource_path_ + "/ad_resource",
        adIndexDir.string(),
        document_manager_, numericTableBuilder_,
        searchManager_->normalSearch_.get(), groupManager_);
    adIndexManager_->buildMiningTask();

    miningTaskBuilder_->addTask(
            adIndexManager_->getMiningTask());
    return true;
}

const std::string& MiningManager::getOfferItemCountPropName_() const
{
    const ProductRankingConfig& rankConfig =
        mining_schema_.product_ranking_config;

    return rankConfig.scores[OFFER_ITEM_COUNT_SCORE].propName;
}

void MiningManager::flush()
{
    LOG(INFO) << "begin flush in MiningManager .... ";
    for (GroupLabelLoggerMap::iterator it = groupLabelLoggerMap_.begin();
            it != groupLabelLoggerMap_.end(); ++it)
    {
        if (it->second)
            it->second->flush();
    }

    LOG(INFO) << "GroupLabelLoggerMap flushed .... ";
    if (customRankManager_) customRankManager_->flush();
    if (merchantScoreManager_) merchantScoreManager_->flush();
    if (idManager_) idManager_->flush();
    LOG(INFO) << "MiningManager flush finished.";
}

void MiningManager::updateMergeFuzzyIndex(int calltype)
{
    if (cronExpression_.matches_now() || calltype > 0)
    {
        static const string cronJobName = "fuzzy_index_merge";
        if (calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(cronJobName, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << cronJobName;
            }
            else
            {
                LOG(INFO) << "cron job on replica ignored. ";
            }
            return;
        }

        DISTRIBUTE_WRITE_BEGIN;
        DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;
        CronJobReqLog reqlog;
        reqlog.cron_time = Utilities::createTimeStamp();
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << cronJobName << std::endl;
            return;
        }
        
        DISTRIBUTE_WRITE_FINISH(true);
    }
}

}

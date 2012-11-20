#include "ProductScoreManager.h"
#include "ProductScoreReader.h"
#include "OfflineProductScorerFactory.h"
#include "ProductScoreTable.h"
#include "ProductScoreReader.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <configuration-manager/ProductRankingConfig.h>
#include <configuration-manager/MiningConfig.h>
#include <document-manager/DocumentManager.h>
#include <util/scheduler.h>
#include <glog/logging.h>

using namespace sf1r;

ProductScoreManager::ProductScoreManager(
    const ProductRankingConfig& config,
    const ProductRankingPara& bundleParam,
    OfflineProductScorerFactory& offlineScorerFactory,
    const DocumentManager& documentManager,
    const std::string& collectionName,
    const std::string& dirPath)
    : config_(config)
    , offlineScorerFactory_(offlineScorerFactory)
    , documentManager_(documentManager)
    , dirPath_(dirPath)
    , cronJobName_("ProductScoreManager-" + collectionName)
{
    createProductScoreTable_(POPULARITY_SCORE);

    addCronJob_(bundleParam);
}

ProductScoreManager::~ProductScoreManager()
{
    for (ScoreTableMap::iterator it = scoreTableMap_.begin();
         it != scoreTableMap_.end(); ++it)
    {
        delete it->second;
    }
}

bool ProductScoreManager::open()
{
    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch (const FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }

    LOG(INFO) << "Start loading product score directory: " << dirPath_;

    for (ScoreTableMap::iterator it = scoreTableMap_.begin();
         it != scoreTableMap_.end(); ++it)
    {
        ProductScoreTable* table = it->second;
        if (!table->open())
        {
            const std::string& typeName =
                ProductRankingConfig::kScoreTypeName[it->first];
            LOG(ERROR) << "ProductScoreTable::open() failed, score type name: "
                       << typeName;
            return false;
        }
    }

    LOG(INFO) << "End loading product score";
    return true;
}

bool ProductScoreManager::buildCollection()
{
    boost::mutex::scoped_lock lock(buildCollectionMutex_);

    return buildCollectionImpl_();
}

bool ProductScoreManager::buildCollectionImpl_()
{
    bool result = true;

    for (ScoreTableMap::iterator it = scoreTableMap_.begin();
         it != scoreTableMap_.end(); ++it)
    {
        if (!buildScoreType_(it->first))
        {
            result = false;
        }
    }

    return result;
}

ProductScorer* ProductScoreManager::createProductScorer(ProductScoreType type)
{
    ScoreTableMap::const_iterator it = scoreTableMap_.find(type);
    if (it == scoreTableMap_.end())
        return NULL;

    const ProductScoreTable* table = it->second;
    return new ProductScoreReader(*table);
}

void ProductScoreManager::createProductScoreTable_(ProductScoreType type)
{
    const ProductScoreConfig& config = config_.scores[type];
    const std::string& typeName = ProductRankingConfig::kScoreTypeName[type];

    if (config.weight != 0)
    {
        scoreTableMap_[type] = new ProductScoreTable(dirPath_, typeName);
    }
}

bool ProductScoreManager::buildScoreType_(ProductScoreType type)
{
    const std::string& typeName = ProductRankingConfig::kScoreTypeName[type];
    const ProductScoreConfig& config = config_.scores[type];
    boost::scoped_ptr<ProductScorer> scorer(
        offlineScorerFactory_.createScorer(config));

    if (!scorer)
    {
        LOG(ERROR) << "failed to create offline scorer for " << typeName;
        return false;
    }

    const docid_t startDocId = 1;
    const docid_t endDocId = documentManager_.getMaxDocId();

    LOG(INFO) << "start building product score: " << typeName
              << ", start doc id: " << startDocId
              << ", end doc id: " << endDocId;

    ProductScoreTable* scoreTable = scoreTableMap_[type];
    scoreTable->resize(endDocId + 1);

    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % 100000 == 0)
        {
            std::cout << "\rbuilding score for doc id: " << docId
                      << "\t" << std::flush;
        }

        score_t score = scorer->score(docId);
        scoreTable->setScore(docId, score);
    }
    std::cout << "\rbuilding score for doc id: " << endDocId
              << "\t" << std::endl;

    if (!scoreTable->flush())
    {
        LOG(ERROR) << "ProductScoreTable::flush() failed, score type: "
                   << typeName;
        return false;
    }

    return true;
}

bool ProductScoreManager::addCronJob_(const ProductRankingPara& bundleParam)
{
    if (!cronExpression_.setExpression(bundleParam.cron))
        return false;

    bool result = izenelib::util::Scheduler::addJob(
        cronJobName_,
        bundleParam.schedule_interval_ms,
        0, // start from now
        boost::bind(&ProductScoreManager::runCronJob_, this));

    if (!result)
    {
        LOG(ERROR) << "failed in izenelib::util::Scheduler::addJob(), "
                   << "cron job name: " << cronJobName_;
    }

    return result;
}

void ProductScoreManager::runCronJob_()
{
    if (cronExpression_.matches_now())
    {
        boost::mutex::scoped_try_lock lock(buildCollectionMutex_);

        if (lock.owns_lock() == false)
        {
            LOG(INFO) << "as still in building collection, exit cron job: "
                      << cronJobName_;
            return;
        }

        buildCollectionImpl_();
    }
}

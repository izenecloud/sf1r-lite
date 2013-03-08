#include "MiningSearchService.h"
#include <mining-manager/MiningManager.h>
#include <mining-manager/faceted-submanager/ontology_manager.h>
#include <node-manager/DistributeRequestHooker.h>

#include <util/driver/Request.h>
#include <aggregator-manager/SearchWorker.h>

using namespace izenelib::driver;
namespace sf1r
{

MiningSearchService::MiningSearchService()
{
}

MiningSearchService::~MiningSearchService()
{
}

bool MiningSearchService::HookDistributeRequest(const std::string& coll, uint32_t workerId)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually.
        return true;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        searchAggregator_->singleRequest(coll, "HookDistributeRequest", (int)hooktype, reqdata, ret, workerId);
    }
    else
    {
        ret = true;
    }
    if (!ret)
    {
        LOG(WARNING) << "Request failed, HookDistributeRequest failed.";
    }
    return ret;
}

bool MiningSearchService::getSearchResult(
    KeywordSearchResult& resultItem
)
{
    return miningManager_->getMiningResult(resultItem);
}

//xxx
bool MiningSearchService::getSimilarDocIdList(
    uint32_t documentId,
    uint32_t maxNum,
    std::vector<std::pair<uint32_t, float> >& result
)
{
    return miningManager_->getSimilarDocIdList(documentId, maxNum, result);
}

bool MiningSearchService::getSimilarDocIdList(
        const std::string& collectionName,
        uint64_t documentId,
        uint32_t maxNum,
        std::vector<std::pair<uint64_t, float> >& result)
{
    if (!bundleConfig_->isMasterAggregator_ || !searchAggregator_->isNeedDistribute())
    {
        searchWorker_->getSimilarDocIdList(documentId, maxNum, result);;
        return true;
    }
    else
    {
        sf1r::workerid_t workerId = net::aggregator::Util::GetWorkerAndDocId(documentId).first;
        return searchAggregator_->singleRequest(collectionName, "getSimilarDocIdList", documentId, maxNum, result, workerId);
    }
}

bool MiningSearchService::getDuplicateDocIdList(
    uint32_t docId,
    std::vector<uint32_t>& docIdList
)
{
    return miningManager_->getDuplicateDocIdList(docId,docIdList);
}

bool MiningSearchService::getSimilarImageDocIdList(
    const std::string& targetImageURI,
    SimilarImageDocIdList& imageDocIdList
)
{
    return true;
}

bool MiningSearchService::getReminderQuery(
    std::vector<izenelib::util::UString>& popularQueries,
    std::vector<izenelib::util::UString>& realtimeQueries
)
{
    return true;
//     return miningManager_->getReminderQuery(popularQueries, realtimeQueries);
}

bool MiningSearchService::getSimilarLabelStringList(
    uint32_t label_id,
    std::vector<izenelib::util::UString>& label_list
)
{
    return miningManager_->getSimilarLabelStringList(label_id, label_list);
}

bool MiningSearchService::getDocLabelList(
    uint32_t docid,
    std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list
)
{
    // TODO, aggregate by wdocid
    return miningManager_->getLabelListByDocId(docid, label_list);
}

bool MiningSearchService::getLabelListWithSimByDocId(
    uint32_t docid,
    std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list
)
{
    // TODO, aggregate by wdocid
    return miningManager_->getLabelListWithSimByDocId(docid, label_list);
}

bool MiningSearchService::getUniqueDocIdList(
    const std::vector<uint32_t>& docIdList,
    std::vector<uint32_t>& cleanDocs
)
{
    return miningManager_->getUniqueDocIdList(docIdList, cleanDocs);
}

bool MiningSearchService::SetOntology(const std::string& xml)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    NoAdditionReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }
    faceted->SetXML(xml);
    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

bool MiningSearchService::GetOntology(std::string& xml)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted->GetXML(xml);
    return true;
}

bool MiningSearchService::GetStaticOntologyRepresentation(faceted::OntologyRep& rep)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->GetStaticRepresentation(rep);
    return true;
}

bool MiningSearchService::OntologyStaticClick(uint32_t cid, std::list<uint32_t>& docid_list)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->StaticClick(cid, docid_list);
    return true;
}

bool MiningSearchService::GetOntologyRepresentation(
        const std::vector<uint32_t>& search_result,
        faceted::OntologyRep& rep)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->GetRepresentation(search_result, rep);
    return true;
}

bool MiningSearchService::OntologyClick(
        const std::vector<uint32_t>& search_result,
        uint32_t cid,
        std::list<uint32_t>& docid_list)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted::OntologySearcher* searcher = faceted->GetSearcher();
    searcher->Click(search_result, cid, docid_list);
    return true;
}

bool MiningSearchService::DefineDocCategory(
        const std::vector<faceted::ManmadeDocCategoryItem>& items)
{
    boost::shared_ptr<faceted::OntologyManager> faceted = miningManager_->GetFaceted();
    if (!faceted)
    {
        return false;
    }
    faceted->DefineDocCategory(items);
    return true;
}

// xxx
bool MiningSearchService::visitDoc(const uint128_t& scdDocId)
{
    uint64_t internalId = 0;
    searchWorker_->getInternalDocumentId(scdDocId, internalId);
    return miningManager_->visitDoc(internalId);
}

bool MiningSearchService::visitDoc(const std::string& collectionName, uint64_t wdocId)
{
    std::pair<sf1r::workerid_t, sf1r::docid_t> wd = net::aggregator::Util::GetWorkerAndDocId(wdocId);
    sf1r::workerid_t workerId = wd.first;
    sf1r::docid_t docId = wd.second;
    bool ret = true;

    if (!bundleConfig_->isMasterAggregator_ || !searchAggregator_->isNeedDistribute())
    {
        searchWorker_->visitDoc(docId, ret);
    }
    else
    {
        if(!HookDistributeRequest(collectionName, workerId))
            return false;
        searchAggregator_->singleRequest(collectionName, "visitDoc", docId, ret, workerId);
    }

    return ret;
}

bool MiningSearchService::clickGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::string>& groupPath)
{
    return miningManager_->clickGroupLabel(query, propName, groupPath);
}

bool MiningSearchService::getFreqGroupLabel(
        const std::string& query,
        const std::string& propName,
        int limit,
        std::vector<std::vector<std::string> >& pathVec,
        std::vector<int>& freqVec)
{
    return miningManager_->getFreqGroupLabel(query, propName, limit, pathVec, freqVec);
}

bool MiningSearchService::setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::vector<std::string> >& groupLabels)
{
    bool result = miningManager_->setTopGroupLabel(query,
                                                   propName, groupLabels);
    if (result)
    {
        searchWorker_->clearSearchCache();
    }

    return result;
}

bool MiningSearchService::setCustomRank(
        const std::string& query,
        const CustomRankDocStr& customDocStr)
{
    bool result = miningManager_->setCustomRank(query, customDocStr);

    searchWorker_->clearSearchCache();

    return result;
}

bool MiningSearchService::getCustomRank(
        const std::string& query,
        std::vector<Document>& topDocList,
        std::vector<Document>& excludeDocList)
{
    return miningManager_->getCustomRank(query, topDocList, excludeDocList);
}

bool MiningSearchService::getCustomQueries(std::vector<std::string>& queries)
{
    return miningManager_->getCustomQueries(queries);
}

bool MiningSearchService::setMerchantScore(const MerchantStrScoreMap& scoreMap)
{
    bool result = miningManager_->setMerchantScore(scoreMap);

    searchWorker_->clearSearchCache();

    return result;
}

bool MiningSearchService::getMerchantScore(
        const std::vector<std::string>& merchantNames,
        MerchantStrScoreMap& merchantScoreMap) const
{
    return miningManager_->getMerchantScore(merchantNames, merchantScoreMap);
}

bool MiningSearchService::GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list)
{
    return miningManager_->GetTdtInTimeRange(start, end, topic_list);
}

bool MiningSearchService::GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& topic_info)
{
    return miningManager_->GetTdtTopicInfo(text, topic_info);
}

bool MiningSearchService::GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit)
{
    return miningManager_->GetTopics(content, topic_list, limit);
}

void MiningSearchService::GetRefinedQuery(const izenelib::util::UString& query, izenelib::util::UString& result)
{
    miningManager_->GetRefinedQuery(query, result);
}

void MiningSearchService::InjectQueryCorrection(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    miningManager_->InjectQueryCorrection(query, result);
}

void MiningSearchService::FinishQueryCorrectionInject()
{
    miningManager_->FinishQueryCorrectionInject();
}

void MiningSearchService::InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    miningManager_->InjectQueryRecommend(query, result);
}

void MiningSearchService::FinishQueryRecommendInject()
{
    miningManager_->FinishQueryRecommendInject();
}

bool MiningSearchService::GetSummarizationByRawKey(
        const std::string& collection,
        const izenelib::util::UString& rawKey,
        Summarization& result)
{
    ///TODO distributed request is not available
    return miningManager_->GetSummarizationByRawKey(rawKey,result);
}

bool MiningSearchService::SetKV(const std::string& key, const std::string& value)
{
    return miningManager_->SetKV(key, value);
}
bool MiningSearchService::GetKV(const std::string& key, std::string& value)
{
    return miningManager_->GetKV(key, value);
}

bool MiningSearchService::GetProductScore(
        const std::string& docIdStr,
        const std::string& scoreType,
        score_t& scoreValue)
{
    return miningManager_->getProductScore(docIdStr, scoreType, scoreValue);
}

bool MiningSearchService::GetProductCategory(
        const std::string& query,
        int limit,
        std::vector<std::vector<std::string> >& pathVec)
{
    return miningManager_->GetProductCategory(query, limit, pathVec);
}

void MiningSearchService::flush()
{
    miningManager_->flush();
}

}

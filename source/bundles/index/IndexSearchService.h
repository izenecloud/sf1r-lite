#ifndef INDEX_BUNDLE_SEARCH_SERVICE_H
#define INDEX_BUNDLE_SEARCH_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>
#include <common/Status.h>

#include <search-manager/SearchManager.h>
#include <ranking-manager/RankingManager.h>
#include <query-manager/ActionItem.h>
#include <index-manager/IndexManager.h>
#include <document-manager/Document.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>
#include <ir/id_manager/IDManager.h>
#include <question-answering/QuestionAnalysis.h>

#include <util/osgi/IService.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class MiningSearchService;
class IndexSearchService : public ::izenelib::osgi::IService
{
public:
    IndexSearchService();

    ~IndexSearchService();

public:
    bool getSearchResult(KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& scdDocumentId, uint32_t& internalId);


private:

    bool buildQuery(
        SearchKeywordOperation& actionOperation,
        std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
        KeywordSearchResult& resultItem
    );

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT& resultItem);

    bool removeDuplicateDocs( 
            KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem);

private:
    IndexBundleConfiguration* bundleConfig_;
    MiningSearchService* miningSearchService_;
    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<RankingManager> rankingManager_;
    boost::shared_ptr<SearchManager> searchManager_;
    ilplib::qa::QuestionAnalysis* pQA_;

    AnalysisInfo analysisInfo_;

    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
};

}

#endif


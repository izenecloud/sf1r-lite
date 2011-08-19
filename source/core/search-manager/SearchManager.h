#ifndef _SEARCH_MANAGER_
#define _SEARCH_MANAGER_

//TODO Bad Smell Here
#include <bundles/index/IndexBundleConfiguration.h>

#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>

#include <ir/id_manager/IDManager.h>
#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/function.hpp>

#include <vector>
#include <deque>
#include <set>

namespace sf1r {

class SearchCache;
class QueryBuilder;
class DocumentManager;
class RankingManager;
class IndexManager;

namespace faceted
{
class GroupFilterBuilder;
class OntologyRep;
}

class SearchManager
{
    typedef izenelib::ir::idmanager::IDManager IDManager;

public:
    typedef boost::function< void( std::vector<unsigned int>&, std::vector<float>&, const std::string& ) > reranker_t;
    SearchManager(
        std::set<PropertyConfig, PropertyComp> schema,
        const boost::shared_ptr<IDManager>& idManager,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        IndexBundleConfiguration* config
    );

    ~SearchManager();

    bool search(SearchKeywordOperation& actionOperation,
                std::vector<unsigned int>& docIdList,
                std::vector<float>& rankScoreList,
                std::vector<float>& customRankScoreList,
                std::size_t& totalCount,
                faceted::OntologyRep& groupRep,
                faceted::OntologyRep& attrRep,
                int topK = 200,
                int start = 0,
                DistKeywordSearchInfo& distSearchInfo = kDefaultDistSearchInfo_);


    void reset_cache(bool rType, docid_t id, const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue);

    /// @brief change working dir by setting new underlying componenets
    void chdir(const boost::shared_ptr<IDManager>& idManager,
               const boost::shared_ptr<DocumentManager>& documentManager,
               const boost::shared_ptr<IndexManager>& indexManager,
               IndexBundleConfiguration* config
               );

    void set_reranker(reranker_t reranker){
        reranker_ = reranker;
    }

    void setGroupFilterBuilder(faceted::GroupFilterBuilder* builder);

private:
    bool doSearch_(SearchKeywordOperation& actionOperation,
                   std::vector<unsigned int>& docIdList,
                   std::vector<float>& rankScoreList,
                   std::vector<float>& customRankScoreList,
                   std::size_t& totalCount,
                   faceted::OntologyRep& groupRep,
                   faceted::OntologyRep& attrRep,
                   int topK,
                   int start,
                   DistKeywordSearchInfo& distSearchInfo  = kDefaultDistSearchInfo_);

    /**
     * @brief get corresponding id of the property, returns 0 if the property
     * does not exist.
     * @see CollectionMeta::numberPropertyConfig
     */
    propertyid_t getPropertyIdByName(const std::string& name) const;

private:
    /**
     * @brief for testing
     */
    void printDFCTF(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties ctfmap);

private:
    boost::unordered_map<std::string, PropertyConfig> schemaMap_;
    boost::shared_ptr<IndexManager> indexManagerPtr_;
    boost::shared_ptr<DocumentManager> documentManagerPtr_;
    boost::shared_ptr<RankingManager> rankingManagerPtr_;
    boost::shared_ptr<QueryBuilder> queryBuilder_;
    std::map<propertyid_t, float> propertyWeightMap_;

    boost::scoped_ptr<SearchCache> cache_;
    SortPropertyCache* pSorterCache_;

    static DistKeywordSearchInfo kDefaultDistSearchInfo_;

    reranker_t reranker_;

    boost::scoped_ptr<faceted::GroupFilterBuilder> groupFilterBuilder_;
};

} // end - namespace sf1r

#endif // _SEARCH_MANAGER_

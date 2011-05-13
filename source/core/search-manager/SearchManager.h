/// @file SearchManager.h
/// @brief header file of SearchManager class
/// @author KyungHoon & Dohyun Yun
/// @date 2008-06-05
/// @details
///     - 2009.08.05 Change the constructor and add indexManagerPtr_
///     - 2009.08.06 Added document manager parameter in the constructor by Dohyun Yun.
///     - 2009.09.23 Change for the new query evaluation flow by Yingfeng Zhang
#if !defined(_SEARCH_MANAGER_)
#define _SEARCH_MANAGER_

#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankingManager.h>
#include <document-manager/DocumentManager.h>
#include "QueryBuilder.h"
#include "Sorter.h"
#include "HitQueue.h"
#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <vector>
#include <deque>
#include <set>

namespace sf1r {

class SearchCache;

class SearchManager
{
    typedef izenelib::ir::idmanager::IDManager IDManager;

public:
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
                int topK = 200,
                int start = 0);


    void reset_cache(bool rType, docid_t id, const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue);

    /// @brief change working dir by setting new underlying componenets
    void chdir(const boost::shared_ptr<IDManager>& idManager,
               const boost::shared_ptr<DocumentManager>& documentManager,
               const boost::shared_ptr<IndexManager>& indexManager,
               IndexBundleConfiguration* config
               );

private:
    bool doSearch_(SearchKeywordOperation& actionOperation,
                   std::vector<unsigned int>& docIdList,
                   std::vector<float>& rankScoreList,
                   std::vector<float>& customRankScoreList,
                   std::size_t& totalCount,
                   int topK,
                   int start);

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
    property_weight_map propertyWeightMap_;

    boost::scoped_ptr<SearchCache> cache_;
    SortPropertyCache* pSorterCache_;
};

} // end - namespace sf1r

#endif // _SEARCH_MANAGER_

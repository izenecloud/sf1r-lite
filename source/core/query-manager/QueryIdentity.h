#ifndef SF1R_QUERY_MANAGER_QUERY_IDENTITY_H
#define SF1R_QUERY_MANAGER_QUERY_IDENTITY_H
/**
 * @file query-manager/QueryIdentity.h
 * @author Ian Yang
 * @date Created <2009-09-29 16:20:35>
 * @date Updated <2009-09-30 10:02:07>
 * @brief identity for a query
 */

#include "ActionItem.h"
#include <glog/logging.h>
namespace sf1r {

struct QueryIdentity
{
    QueryIdentity()
        : rankingType(RankingType::DefaultTextRanker)
        , removeDuplicatedDocs(false)
        , start(0)
        , distActionType(0)
        , isRandomRank(false)
        , isSynonym(false)
        , isAnalyzeResult(false)
		, scope(0.0)
    {
    }

    /// @brief original query string
    std::string query;

    /// @brief expanded query string
    std::string expandedQueryString;

    /// @brief user id, todo, whole user info may be needed.
    std::string userId;

    /// @brief searching mode information
    SearchingModeInfo searchingMode;

    /// @brief ranking type, should be sorting mechanism in future
    RankingType::TextRankingType rankingType;

    LanguageAnalyzerInfo laInfo;

    /// @brief searched properties concatenated.
    std::vector<std::string> properties;

    std::vector<std::string> counterList;

    std::vector<std::pair<std::string , bool> > sortInfo;

    ConditionsNode filterTree;

    /// @brief param for group filter
    faceted::GroupParam groupParam;

    std::pair<double, double> geoLocation;
	double					  scope;
	std::string				  geohash;

    /// @brief remove duplicate
    bool removeDuplicatedDocs;

    /// @brief property name which needs range result
    std::string rangeProperty;

    /// @brief custom ranking info
    std::string strExp;
    std::map<std::string, double> paramConstValueMap;
    std::map<std::string, std::string> paramPropertyValueMap;

    std::vector<uint64_t> simHash;

    /// search results offset after topK
    uint32_t start;

    /// action type of distributed search
    int8_t distActionType;

    /// @brief whether rank randomly
    bool isRandomRank;

    /// @brief whether use synonym in search
    bool isSynonym;

    /// @brief If true, return "analyzer_result" in fuzzy search response,
    /// which contains the tokenized result of the query
    bool isAnalyzeResult;

    /// @brief where does the query come from, used to decide
    ///        the categories to boost in product ranking.
    std::string querySource;

    inline bool operator==(const QueryIdentity& other) const
    {
        return rankingType == other.rankingType
            && searchingMode == other.searchingMode
            && simHash == other.simHash
            && query == other.query
            && userId == other.userId
            && laInfo == other.laInfo
            && properties == other.properties
            && counterList == other.counterList
            && sortInfo == other.sortInfo
            && filterTree == other.filterTree
            && groupParam == other.groupParam
			&& scope == other.scope
			&& geohash == other.geohash
            && removeDuplicatedDocs == other.removeDuplicatedDocs
            && rangeProperty == other.rangeProperty
            && strExp == other.strExp
            && paramConstValueMap == other.paramConstValueMap
            && paramPropertyValueMap == other.paramPropertyValueMap
            && start == other.start
            && distActionType == other.distActionType
            && isRandomRank == other.isRandomRank
            && isSynonym == other.isSynonym
            && isAnalyzeResult == other.isAnalyzeResult
            && querySource == other.querySource;
    }

    inline bool operator!=(const QueryIdentity& other) const
    {
        return !operator==(other);
    }

    DATA_IO_LOAD_SAVE(QueryIdentity, & query & userId & searchingMode & rankingType & laInfo
            & properties & counterList & sortInfo & groupParam & scope & geohash & removeDuplicatedDocs
            & rangeProperty & strExp & paramConstValueMap & paramPropertyValueMap & simHash
            & start & distActionType & isRandomRank & isSynonym & isAnalyzeResult & querySource);
};

} // namespace sf1r

MAKE_FEBIRD_SERIALIZATION(sf1r::QueryIdentity)

#endif // SF1R_QUERY_MANAGER_QUERY_IDENTITY_H

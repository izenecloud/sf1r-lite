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

namespace sf1r {

struct QueryIdentity
{
    QueryIdentity()
        : rankingType(RankingType::DefaultTextRanker)
        , start(0)
        , distActionType(0)
        , searchingMode(0)
    {
    }

    /// @brief original query string
    std::string query;

    /// @brief expanded query string
    std::string expandedQueryString;

    /// @brief user id, todo, whole user info may be needed.
    std::string userId;

    /// @brief ranking type, should be sorting mechanism in future
    RankingType::TextRankingType rankingType;

    LanguageAnalyzerInfo laInfo;

    /// @brief searched properties concatenated.
    std::vector<std::string> properties;

    std::vector<std::pair<std::string , bool> > sortInfo;

    std::vector<QueryFiltering::FilteringType> filterInfo;

    /// @brief param for group filter
    faceted::GroupParam groupParam;

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

    int8_t searchingMode;

    inline bool operator==(const QueryIdentity& other) const
    {
        return rankingType == other.rankingType
            && searchingMode == other.searchingMode
            && simHash == other.simHash
            && query == other.query
            && userId == other.userId
            && laInfo == other.laInfo
            && properties == other.properties
            && sortInfo == other.sortInfo
            && filterInfo == other.filterInfo
            && groupParam == other.groupParam
            && rangeProperty == other.rangeProperty
            && strExp == other.strExp
            && paramConstValueMap == other.paramConstValueMap
            && paramPropertyValueMap == other.paramPropertyValueMap
            && start == other.start
            && distActionType == other.distActionType;
    }

    inline bool operator!=(const QueryIdentity& other) const
    {
        return !operator==(other);
    }

    DATA_IO_LOAD_SAVE(QueryIdentity, & query & userId & rankingType & laInfo
            & properties & sortInfo & filterInfo & groupParam & rangeProperty
            & strExp & paramConstValueMap & paramPropertyValueMap & simHash
            & start & distActionType & searchingMode);

    template<class Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & query;
        ar & userId;
        ar & rankingType;
        ar & laInfo;
        ar & properties;
        ar & sortInfo;
        ar & filterInfo;
        ar & groupParam;
        ar & rangeProperty;
        ar & strExp;
        ar & paramConstValueMap;
        ar & paramPropertyValueMap;
        ar & simHash;
        ar & start;
        ar & distActionType;
        ar & searchingMode;
    }
};

} // namespace sf1r
#endif // SF1R_QUERY_MANAGER_QUERY_IDENTITY_H

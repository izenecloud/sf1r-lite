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

    /// search results offset after topK
    int start;

    /// action type of distributed search
    int distActionType;

    DATA_IO_LOAD_SAVE(QueryIdentity, &query&userId&rankingType&laInfo&properties
            &sortInfo&filterInfo&groupParam&rangeProperty
            &strExp&paramConstValueMap&paramPropertyValueMap&start&distActionType);

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
        ar & start;
        ar & distActionType;
    }
};

void makeQueryIdentity(
    QueryIdentity& identity,
    const KeywordSearchActionItem& item,
    int distActionType = 0,
    int start = 0
);

inline bool operator==(const QueryIdentity& a,
                       const QueryIdentity& b)
{
    return a.rankingType == b.rankingType
        && a.query == b.query
        && a.userId == b.userId
        && a.laInfo == b.laInfo
        && a.properties == b.properties
        && a.sortInfo == b.sortInfo
        && a.filterInfo == b.filterInfo
        && a.groupParam == b.groupParam
        && a.rangeProperty == b.rangeProperty
        && a.strExp == b.strExp
        && a.paramConstValueMap == b.paramConstValueMap
        && a.paramPropertyValueMap == b.paramPropertyValueMap
        && a.start == b.start
        && a.distActionType == b.distActionType;
}

inline bool operator!=(const QueryIdentity& a,
                       const QueryIdentity& b)
{
    return !(a == b);
}

} // namespace sf1r
#endif // SF1R_QUERY_MANAGER_QUERY_IDENTITY_H

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

    /// @brief ranking type, should be sorting mechanism in future
    RankingType::TextRankingType rankingType;

    LanguageAnalyzerInfo laInfo;

    /// @brief searched properties concatenated.
    std::vector<std::string> properties;

    std::vector<std::pair<std::string , bool> > sortInfo;

    std::vector<QueryFiltering::FilteringType> filterInfo;
    /// search results offset after topK
    int start;

    DATA_IO_LOAD_SAVE(QueryIdentity, &query&rankingType&laInfo&properties&sortInfo&filterInfo&start);

    template<class Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & query;
        ar & rankingType;
        ar & laInfo;
        ar & properties;
        ar & sortInfo;
        ar & filterInfo;
        ar & start;
    }
};

void makeQueryIdentity(
    QueryIdentity& identity,
    const KeywordSearchActionItem& item,
    int start = 0
);

inline bool operator==(const QueryIdentity& a,
                       const QueryIdentity& b)
{
    return a.rankingType == b.rankingType
        && a.query == b.query
        && a.laInfo == b.laInfo
        && a.properties == b.properties
        && a.sortInfo == b.sortInfo
        && a.filterInfo == b.filterInfo
        && a.start == b.start;
}

inline bool operator!=(const QueryIdentity& a,
                       const QueryIdentity& b)
{
    return !(a == b);
}

} // namespace sf1r
#endif // SF1R_QUERY_MANAGER_QUERY_IDENTITY_H

/**
 * @file SearchBase.h
 * @brief the search interface.
 * @author Jun Jiang
 * @date Created 2013-01-11
 */

#ifndef SF1R_SEARCH_BASE_H
#define SF1R_SEARCH_BASE_H

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class MiningManager;
class SearchKeywordOperation;
class KeywordSearchResult;

class SearchBase
{
public:
    virtual ~SearchBase() {}

    virtual void setMiningManager(
        const boost::shared_ptr<MiningManager>& miningManager)
    {}

    /**
     * get search results.
     *
     * @param actionOperation the search parameters
     * @param searchResult it stores the search results
     * @param limit at most how many docs to return. Generally it's TOP_K_NUM.
     * @param offset the index offset of the first returned doc in all candidate
     *        docs, start from 0. Generally it's a multiple of TOP_K_NUM.
     * @return true for success, false for failure.
     *
     * @note in SF1R driver API documents/search(), there are also such
     *       parameters as "offset" and "limit", which are used for pagination.
     *
     *       While the @p offset and @p limit of this method have different
     *       meanings, they are used to get a batch of docs.
     *
     *       By putting the batch into cache, there is no need to call this
     *       method again as long as the cache is hit.
     */
    virtual bool search(
        const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult,
        std::size_t limit,
        std::size_t offset) = 0;
};

} // namespace sf1r

#endif // SF1R_SEARCH_BASE_H

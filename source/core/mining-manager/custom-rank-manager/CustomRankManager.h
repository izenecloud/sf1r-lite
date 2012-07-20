/**
 * @file CustomRankManager.h
 * @brief set/get the docs custom ranking.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-17
 */

#ifndef SF1R_CUSTOM_RANK_MANAGER_H
#define SF1R_CUSTOM_RANK_MANAGER_H

#include <common/inttypes.h>
#include <common/SDBWrapper.h>

#include <vector>
#include <string>

namespace sf1r
{
class CustomRankScorer;

class CustomRankManager
{
public:
    typedef std::vector<docid_t> DocIdList;

    /**
     * @brief Constructor
     * @param dbPath the db path
     */
    CustomRankManager(const std::string& dbPath);

    void flush();

    /**
     * Set the doc id list which is customized for @p query.
     * @param query user query
     * @param docIdList the doc id list to set
     * @return true for success, false for failure
     */
    bool setDocIdList(
        const std::string& query,
        const DocIdList& docIdList
    );

    /**
     * get the doc id list which is customized for @p query.
     * @param query user query
     * @param docIdList the doc id list to get
     * @return true for success, false for failure
     */
    bool getDocIdList(
        const std::string& query,
        DocIdList& docIdList
    );

    /**
     * get the scorer which is customized for @p query.
     * @param query user query
     * @return the scorer, it would be NULL if no doc id list
     *         is customized for @p query.
     * @attention the caller is responsible to delete the scorer.
     */
    CustomRankScorer* getScorer(const std::string& query);

    /**
     * get the @p queries which have been customized by @c setDocIdList() with
     * non-empty @p docIdList.
     * @return true for success, false for failure
     */
    bool getQueries(std::vector<std::string>& queries);

private:
    /** key: query */
    typedef SDBWrapper<std::string, DocIdList> DBType;

    DBType db_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_MANAGER_H

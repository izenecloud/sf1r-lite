/**
 * @file CustomRankManager.h
 * @brief set/get the docs custom ranking.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-17
 */

#ifndef SF1R_CUSTOM_RANK_MANAGER_H
#define SF1R_CUSTOM_RANK_MANAGER_H

#include "CustomRankValue.h"
#include <common/SDBWrapper.h>

#include <vector>
#include <string>

namespace sf1r
{
class CustomRankScorer;

class CustomRankManager
{
public:
    /**
     * @brief Constructor
     * @param dbPath the db path
     */
    CustomRankManager(const std::string& dbPath);

    void flush();

    /**
     * Set the customized value for @p query.
     * @param query user query
     * @param customRankValue the value to customize
     * @return true for success, false for failure
     */
    bool setCustomValue(
        const std::string& query,
        const CustomRankValue& customValue
    );

    /**
     * get the value which is customized for @p query.
     * @param query user query
     * @param customValue the customized value
     * @return true for success, false for failure
     */
    bool getCustomValue(
        const std::string& query,
        CustomRankValue& customValue
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
     * get the @p queries which have been customized by @c setCustomValue() with
     * non-empty @p customValue.
     * @return true for success, false for failure
     */
    bool getQueries(std::vector<std::string>& queries);

private:
    /** key: query */
    typedef SDBWrapper<std::string, CustomRankValue> DBType;

    DBType db_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_MANAGER_H

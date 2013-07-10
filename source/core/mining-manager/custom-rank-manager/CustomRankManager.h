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
class CustomDocIdConverter;
class DocumentManager;

class CustomRankManager
{
public:
    /**
     * @brief Constructor
     * @param dbPath the db path
     */
    CustomRankManager(
        const std::string& dbPath,
        CustomDocIdConverter& docIdConverter,
        DocumentManager* docManager = NULL
    );

    void flush();

    /**
     * Set the customized value for @p query.
     * @param query user query
     * @param customDocStr the value to customize
     * @return true for success, false for failure
     */
    bool setCustomValue(
        const std::string& query,
        const CustomRankDocStr& customDocStr
    );

    /**
     * get the value which is customized for @p query.
     * @param query user query
     * @param customDocId the customized value
     * @return true for success, false for failure
     */
    bool getCustomValue(
        const std::string& query,
        CustomRankDocId& customDocId
    );

    /**
     * get the @p queries which have been customized by @c setCustomValue() with
     * non-empty @p customValue.
     * @return true for success, false for failure
     */
    bool getQueries(std::vector<std::string>& queries);

private:
    void removeDeletedDocs_(std::vector<docid_t>& docIds);

private:
    /** key: query */
    typedef SDBWrapper<std::string, CustomRankDocStr> DBType;
    DBType db_;

    CustomDocIdConverter& docIdConverter_;

    DocumentManager* docManager_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_MANAGER_H

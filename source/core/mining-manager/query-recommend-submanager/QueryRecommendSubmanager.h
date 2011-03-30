///
/// @file QueryRecommendSubmanager.h
/// @brief The header file of query recommend submanager
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-06-10
/// @date Updated 2009-11-21 00:50:31
/// @date Refined 2009-11-24 Jinglei
///

#ifndef QUERYRECOMMENDSUBMANAGER_H_
#define QUERYRECOMMENDSUBMANAGER_H_

#include <vector>
#include <cmath>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <la/stem/Stemmer.h>
#include <common/type_defs.h>
#include <mining-manager/MiningManagerDef.h>
#include "RecommendManager.h"
#include "QueryRecommendRep.h"
namespace sf1r
{

/**
 * The class interface to recommend query.
 */
class QueryRecommendSubmanager : public boost::noncopyable
{
public:

    QueryRecommendSubmanager(const boost::shared_ptr<RecommendManager>& rmDb);
    ~QueryRecommendSubmanager();


public:


    /**
     * @brief Get the ranked query recommendation list for a given query. It will also consider the result of taxonomy generation.
     * @param queryStr the submitted query
     * @param topDocIdList the top docs from the search result
     * @param maxNum the max number of recommended items
     * @param queryList the resulted recommended items
     */
    void getRecommendQuery(const izenelib::util::UString& queryStr,
                           const std::vector<docid_t>& topDocIdList, unsigned int maxNum,
                           QueryRecommendRep& queryList);


    /**
     * @brief Get the ranked query recommendation list for a given query with the session information
     * @param session the session identifier
     * @param queryStr the submitted query
     * @param topDocIdList the top docs from the search result
     * @param maxNum the max number of recommended items
     * @param queryList the resulted recommended items
     */
    bool getReminderQuery(std::vector<izenelib::util::UString>& popularQueries, std::vector<izenelib::util::UString>& realTimeQueries);



private:

    boost::shared_ptr<RecommendManager> rmDb_;


};

}

#endif /* QUERYRECOMMENDSUBMANAGER_H_ */

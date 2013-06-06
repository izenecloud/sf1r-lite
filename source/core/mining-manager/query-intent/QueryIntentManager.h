/**
 * @file QueryIntentManager.h
 * @brief 
 * @author Kevin Lin
 * @date Created 2013-06-05
 */
#ifndef SF1R_QUERY_INTENT_MANAGER_H
#define SF1R_QUERY_INTENT_MANAGER_H

#include "QueryIntent.h"
#include "QueryIntentFactory.h"
#include "Classifier.h"
#include "ClassifierFactory.h"

#include <util/driver/Request.h>
#include <configuration-manager/QueryIntentConfig.h>
namespace sf1r
{

class QueryIntentManager
{
public:
    QueryIntentManager(QueryIntentConfig* config);
    ~QueryIntentManager();
public:
    void queryIntent(izenelib::driver::Request& request);   
private:
    QueryIntentFactory* intentFactory_;
    ClassifierFactory* classifierFactory_;
    QueryIntentConfig* config_;
};

}
#endif //QueryIntentManager.h

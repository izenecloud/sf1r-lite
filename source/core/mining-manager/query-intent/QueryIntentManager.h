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
#include <configuration-manager/CollectionPath.h>

#include <sys/time.h>
#define QUERY_INTENT_TIMER
namespace sf1r
{

class QueryIntentManager
{
public:
    QueryIntentManager(QueryIntentConfig* config, std::string& resource);
    ~QueryIntentManager();
public:
    void queryIntent(izenelib::driver::Request& request);
    void reload();
private:
    void init_();
    void destroy_();
private:
    QueryIntentFactory* intentFactory_;
    ClassifierFactory* classifierFactory_;
    QueryIntent* intent_;
    
    QueryIntentConfig* config_;
    std::string lexiconDirectory_;
#ifdef QUERY_INTENT_TIMER    
    unsigned int nQuery_;
    unsigned long int microseconds_;
#endif
};

}
#endif //QueryIntentManager.h

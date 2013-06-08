#include "QueryIntentManager.h"

namespace sf1r
{

QueryIntentManager::QueryIntentManager(QueryIntentConfig* config, std::string& resource)
{
    config_ = config;
    lexiconDirectory_ = resource + "/query-intent/";
#ifdef QUERY_INTENT_TIMER    
    nQuery_ = 0;
    microseconds_ = 0;
#endif
    init_();
}

QueryIntentManager::~QueryIntentManager()
{
#ifdef QUERY_INTENT_TIMER    
    double rate = (double)microseconds_ / nQuery_;
    std::cout<<"==========================================================\n";
    std::cout<<"| QueryIntent rate = "<<rate<<"(microseconds per query)  |\n";
    std::cout<<"==========================================================\n";
#endif
    destroy_();
}

void QueryIntentManager::init_()
{
    intentFactory_ = new QueryIntentFactory();
    classifierFactory_ = new ClassifierFactory(*config_);
   
    ClassifierContext cContext(*config_, lexiconDirectory_);
    Classifier* classifier = classifierFactory_->createClassifier(cContext);
    
    QueryIntentContext iContext;
    intent_ = intentFactory_ ->createQueryIntent(iContext);

    intent_->addClassifier(classifier);
}

void QueryIntentManager::destroy_()
{
    if (intent_)
    {
        delete intent_;
        intent_ = NULL;
    }
    if (intentFactory_)
    {
        delete intentFactory_;
        intentFactory_ = NULL;
    }
    if (classifierFactory_)
    {
        delete classifierFactory_;
        classifierFactory_ = NULL;
    }
}

void QueryIntentManager::queryIntent(izenelib::driver::Request& request)
{
#ifdef QUERY_INTENT_TIMER    
    struct timeval begin;
    gettimeofday(&begin, NULL);
#endif
    if (intent_)
        intent_->process(request);
#ifdef QUERY_INTENT_TIMER    
    struct timeval end;
    gettimeofday(&end, NULL);
    nQuery_++;
    microseconds_ = 1e6 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec);
#endif
}

}

#include "QueryIntentManager.h"

namespace sf1r
{

QueryIntentManager::QueryIntentManager(QueryIntentConfig* config, std::string& resource, MiningManager* miningManager)
{
    config_ = config;
    lexiconDirectory_ = resource + "/query-intent/";
    miningManager_ = miningManager;
#ifdef QUERY_INTENT_TIMER    
    nQuery_ = 0;
    nanoseconds_ = 0;
#endif
    init_();
}

QueryIntentManager::~QueryIntentManager()
{
#ifdef QUERY_INTENT_TIMER    
    double rate = (double)nanoseconds_ / nQuery_;
    std::cout<<"==========================================================\n";
    std::cout<<"| QueryIntent rate = "<<rate<<"(nanoseconds per query)  |\n";
    std::cout<<"==========================================================\n";
#endif
    destroy_();
}

void QueryIntentManager::init_()
{
    intentFactory_ = new QueryIntentFactory();
    
    IntentContext* iContext = new IntentContext(config_);
    intent_ = intentFactory_ ->createQueryIntent(iContext);

    classifierFactory_ = new ClassifierFactory();

    QIIterator it = config_->begin();
    for (; it != config_->end(); it++)
    {
        ClassifierContext* cContext = new ClassifierContext(config_, lexiconDirectory_, miningManager_, it->type_, it->name_);
        Classifier* classifier = classifierFactory_->createClassifier(cContext);
        if (classifier)
            intent_->addClassifier(classifier);
    }
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

void QueryIntentManager::queryIntent(
    izenelib::driver::Request& request,
    izenelib::driver::Response& response)
{
#ifdef QUERY_INTENT_TIMER
    int clk_id = CLOCK_REALTIME;
    struct timespec begin;
    clock_gettime(clk_id, &begin);
#endif
    if (intent_)
        intent_->process(request, response);
#ifdef QUERY_INTENT_TIMER    
    struct timespec end;
    clock_gettime(clk_id, &end);
    nQuery_++;
    nanoseconds_ = 1e9 * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
#endif
}

void QueryIntentManager::reload()
{
    if(intent_)
        intent_->reloadLexicon();
}

}

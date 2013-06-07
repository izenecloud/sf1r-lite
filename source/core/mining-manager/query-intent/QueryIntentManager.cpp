#include "QueryIntentManager.h"

namespace sf1r
{

QueryIntentManager::QueryIntentManager(QueryIntentConfig* config, std::string& resource)
{
    config_ = config;
    lexiconDirectory_ = resource + "/query-intent/";
    init_();
}

QueryIntentManager::~QueryIntentManager()
{
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
    if (intent_)
        intent_->process(request);
}

}

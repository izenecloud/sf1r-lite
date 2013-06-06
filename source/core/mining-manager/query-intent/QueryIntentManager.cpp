#include "QueryIntentManager.h"

namespace sf1r
{

QueryIntentManager::QueryIntentManager(QueryIntentConfig* config)
{
    config_ = config;
    intentFactory_ = new QueryIntentFactory();
    classifierFactory_ = new ClassifierFactory(config);
}

QueryIntentManager::~QueryIntentManager()
{
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
    ClassifierContext cContext;
    Classifier* classifier = classifierFactory_->createClassifier(cContext);

    QueryIntentContext iContext;
    QueryIntent* intent = intentFactory_ ->createQueryIntent(iContext);

    intent->setClassifier(classifier);
    intent->process(request);

    delete classifier;
    classifier = NULL;
    delete intent;
    intent = NULL;
}

}

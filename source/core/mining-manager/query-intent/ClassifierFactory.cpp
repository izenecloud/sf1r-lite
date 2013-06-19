#include "ClassifierFactory.h"
#include "ProductClassifier.h"
#include "LexiconClassifier.h"
#include "TrieClassifier.h"
#include "LoggerClassifier.h"

#include <string>

namespace sf1r
{
Classifier* ClassifierFactory::createClassifier(ClassifierContext* context)
{
    std::string type;
    if ((type = LexiconClassifier::type()) == context->type_)
    {
        Classifier* classifier = new LexiconClassifier(context);
        classifier->loadLexicon();
        return classifier;
    }
    if ((type = ProductClassifier::type()) == context->type_)
    {
        return new ProductClassifier(context);
    }
    if ((type = LoggerClassifier::type()) == context->type_)
    {
        return new LoggerClassifier(context);
    }
    if ((type = TrieClassifier::type()) == context->type_)
    {
        Classifier* classifier = new TrieClassifier(context);
        classifier->loadLexicon();
        return classifier;
    }
    else
        return NULL;
}
}

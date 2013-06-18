#include "ClassifierFactory.h"
#include "ProductClassifier.h"
#include "LexiconClassifier.h"
#include "LoggerClassifier.h"

#include <string>

namespace sf1r
{
Classifier* ClassifierFactory::createClassifier(ClassifierContext* context)
{
    std::string type;
    if ((type = LexiconClassifier::type()) == context->type_)
        return new LexiconClassifier(context);
    if ((type = ProductClassifier::type()) == context->type_)
        return new ProductClassifier(context);
    if ((type = LoggerClassifier::type()) == context->type_)
        return new LoggerClassifier(context);
    else
        return NULL;
}
}

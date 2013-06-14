#include "ClassifierFactory.h"
#include "QueryCategoryClassifier.h"
#include "SourceClassifier.h"

#include <string>

namespace sf1r
{
Classifier* ClassifierFactory::createClassifier(ClassifierContext* context)
{
    std::string type;
    if ((type = SourceClassifier::type()) == context->type_)
        return new SourceClassifier(context);
    if ((type = QueryCategoryClassifier::type()) == context->type_)
        return new QueryCategoryClassifier(context);
    else
        return NULL;
}
}

#include "ClassifierFactory.h"
#include "NaiveBayesClassifier.h"
#include "LexiconClassifier.h"

namespace sf1r
{
Classifier* ClassifierFactory::createClassifier(ClassifierContext& context)
{
    return new LexiconClassifier(context);
}
}

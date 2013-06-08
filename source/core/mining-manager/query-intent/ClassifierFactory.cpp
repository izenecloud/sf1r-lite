#include "ClassifierFactory.h"
#include "NaiveBayesClassifier.h"

namespace sf1r
{
Classifier* ClassifierFactory::createClassifier(ClassifierContext& context)
{
    return new NaiveBayesClassifier();
}
}

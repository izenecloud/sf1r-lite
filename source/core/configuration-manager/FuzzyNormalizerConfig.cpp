#include "FuzzyNormalizerConfig.h"

using namespace sf1r;

FuzzyNormalizerConfig::FuzzyNormalizerConfig()
        : type(ALPHA_NUM_NORMALIZER)
        , maxIndexToken(0)
{
    typeMap["alphanum"] = ALPHA_NUM_NORMALIZER;
    typeMap["token"] = TOKEN_NORMALIZER;
}

FuzzyNormalizerType FuzzyNormalizerConfig::getNormalizerType(const std::string& typeName) const
{
    TypeMap::const_iterator it = typeMap.find(typeName);

    if (it != typeMap.end())
        return it->second;

    return FUZZY_NORMALIZER_NUM;
}

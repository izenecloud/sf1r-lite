#include "FuzzyNormalizerFactory.h"
#include "FuzzyAlphaNumNormalizer.h"
#include "FuzzyTokenNormalizer.h"
#include "ProductTokenizer.h"
#include <configuration-manager/FuzzyNormalizerConfig.h>
#include <glog/logging.h>

using namespace sf1r;

FuzzyNormalizerFactory::FuzzyNormalizerFactory(ProductTokenizer* productTokenizer)
        : productTokenizer_(productTokenizer)
{
}

FuzzyNormalizer* FuzzyNormalizerFactory::createFuzzyNormalizer(const FuzzyNormalizerConfig& config)
{
    switch (config.type)
    {
    case ALPHA_NUM_NORMALIZER:
        return new FuzzyAlphaNumNormalizer;

    case TOKEN_NORMALIZER:
        return new FuzzyTokenNormalizer(productTokenizer_,
                                        config.maxIndexToken);

    default:
        LOG(WARNING) << "unknown fuzzy normalizer type " << config.type;
        return NULL;
    }
}

/**
 * @file FuzzyNormalizerFactory.h
 * @brief create FuzzyNormalizer instance.
 */

#ifndef FUZZY_NORMALIZER_FACTORY_H
#define FUZZY_NORMALIZER_FACTORY_H

namespace sf1r
{
class FuzzyNormalizerConfig;
class ProductTokenizer;
class FuzzyNormalizer;

class FuzzyNormalizerFactory
{
public:
    FuzzyNormalizerFactory(ProductTokenizer* productTokenizer);

    FuzzyNormalizer* createFuzzyNormalizer(const FuzzyNormalizerConfig& config);

private:
    ProductTokenizer* productTokenizer_;
};

}

#endif // FUZZY_NORMALIZER_FACTORY_H

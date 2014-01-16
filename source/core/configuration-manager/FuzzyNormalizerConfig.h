#ifndef SF1R_FUZZY_NORMALIZER_CONFIG_H_
#define SF1R_FUZZY_NORMALIZER_CONFIG_H_

#include <string>
#include <map>
#include <boost/serialization/access.hpp>

namespace sf1r
{

enum FuzzyNormalizerType
{
    ALPHA_NUM_NORMALIZER = 0,
    TOKEN_NORMALIZER,
    FUZZY_NORMALIZER_NUM
};

/**
 * @brief The configuration for <SuffixMatch> <Normalizer>.
 */
class FuzzyNormalizerConfig
{
public:
    FuzzyNormalizerType type;

    int maxIndexToken;

    FuzzyNormalizerConfig();

    FuzzyNormalizerType getNormalizerType(const std::string& typeName) const;

private:
    /** key: score type name */
    typedef std::map<std::string, FuzzyNormalizerType> TypeMap;
    TypeMap typeMap;

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type;
        ar & typeMap;
    }
};

} // namespace sf1r

#endif // SF1R_FUZZY_NORMALIZER_CONFIG_H_

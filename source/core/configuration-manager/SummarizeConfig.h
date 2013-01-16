#ifndef SF1R_SUMMARIZATION_CONFIG_H_
#define SF1R_SUMMARIZATION_CONFIG_H_

#include <string>

#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class SummarizeConfig
{
public:
    /// property name
    std::string docidPropName;
    std::string uuidPropName;
    std::string contentPropName;
    std::string advantagePropName;
    std::string disadvantagePropName;
    std::string titlePropName;
    std::string scorePropName;
    std::string commentCountPropName;
    std::string opinionPropName;
    std::string opinionWorkingPath;
    std::string opinionSyncId;
    bool isForSyncFullSCD;

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & docidPropName;
        ar & uuidPropName;
        ar & contentPropName;
        ar & advantagePropName;
        ar & disadvantagePropName;
        ar & titlePropName;
        ar & scorePropName;
        ar & commentCountPropName;
        ar & opinionPropName;
        ar & opinionWorkingPath;
        ar & opinionSyncId;
        ar & isForSyncFullSCD;
    }
};

} // namespace

#endif //_ATTR_CONFIG_H_

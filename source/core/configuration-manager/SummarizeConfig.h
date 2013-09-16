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
    SummarizeConfig()
        :isSyncSCDOnly(false), recent_days(30)
    {
    }
    /// property name
    std::string docidPropName;
    std::string uuidPropName;
    std::string contentPropName;
    std::string advantagePropName;
    std::string disadvantagePropName;
    std::string titlePropName;
    std::string scorePropName;
    std::string commentCountPropName;
    std::string recentCommentCountPropName;
    std::string opinionPropName;
    std::string opinionWorkingPath;
    std::string opinionSyncId;
    bool isSyncSCDOnly;
    int32_t recent_days;

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
        ar & recentCommentCountPropName;
        ar & opinionPropName;
        ar & opinionWorkingPath;
        ar & opinionSyncId;
        ar & isSyncSCDOnly;
        ar & recent_days;
    }
};

} // namespace

#endif //_ATTR_CONFIG_H_

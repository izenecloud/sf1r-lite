#ifndef SF1R_GROUP_CONFIG_H_
#define SF1R_GROUP_CONFIG_H_

#include <mining-manager/faceted-submanager/ontology_rep.h>

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of group property configuration.
 */
class GroupConfig
{
public:
    /// property name
    std::string propName;

    /// the value tree,
    /// the level of root value starts from 1,
    /// so that these nodes could be used in group result directly
    faceted::OntologyRep valueTree;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propName;
        ar & valueTree;
    }
};

} // namespace

#endif //_GROUP_CONFIG_H_

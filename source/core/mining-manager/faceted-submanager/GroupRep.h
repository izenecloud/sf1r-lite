///
/// @file GroupRep.h
/// @brief Group-by representation class
/// @author August Njam Grong <longran1989@gmail.com>
/// @date Created 2011-09-23
///

#ifndef SF1R_GROUP_REP_H_
#define SF1R_GROUP_REP_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_rep.h"

NS_FACETED_BEGIN

using namespace std;

class GroupRep
{
public:
    GroupRep();
    ~GroupRep();

    void swap(GroupRep& other);
    void merge(const GroupRep& other);
    void convertGroupRep();
    string ToString() const;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & stringGroupRep_;
        ar & numericGroupRep_;
        ar & numericRangeGroupRep_;
    }

    MSGPACK_DEFINE(
        stringGroupRep_,
        numericGroupRep_,
        numericRangeGroupRep_
    );

private:
    void mergeStringGroup(const GroupRep& other);
    void mergeNumericGroup(const GroupRep& other);
    void mergeNumericRangeGroup(const GroupRep& other);

public:
    typedef list<list<OntologyRepItem> > StringGroupRep;
    typedef list<pair<string, map<double, unsigned int> > > NumericGroupRep;
    typedef list<pair<string, vector<unsigned int> > > NumericRangeGroupRep;

    StringGroupRep stringGroupRep_;
    NumericGroupRep numericGroupRep_;
    NumericRangeGroupRep numericRangeGroupRep_;
};

NS_FACETED_END

#endif

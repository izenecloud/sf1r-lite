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
#include "../faceted-submanager/faceted_types.h"
#include "../faceted-submanager/ontology_rep_item.h"

NS_FACETED_BEGIN

class GroupRep
{
public:
    GroupRep();
    ~GroupRep();

    typedef std::list<OntologyRepItem> StringGroupRep;
    typedef std::list<std::pair<std::string, std::list<std::pair<double, unsigned int> > > > NumericGroupRep;
    typedef std::list<std::pair<std::string, std::vector<unsigned int> > > NumericRangeGroupRep;

    bool empty() const;
    void clear();
    void swap(GroupRep& other);
    bool operator==(const GroupRep& other) const;

    void merge(const GroupRep& other);
    string ToString() const;

    template<typename CounterType> friend class NumericGroupCounter;
    friend class NumericRangeGroupCounter;
    void toOntologyRepItemList();

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

    static void formatNumericToUStr(double value, izenelib::util::UString& ustr);

private:
    void mergeStringGroup(const GroupRep& other);
    void mergeNumericGroup(const GroupRep& other);
    void mergeNumericRangeGroup(const GroupRep& other);

    void recurseStringGroup(
        const GroupRep& other,
        StringGroupRep::iterator &it,
        StringGroupRep::const_iterator &oit,
        uint8_t level
    );

public:
    StringGroupRep stringGroupRep_;

private:
    NumericGroupRep numericGroupRep_;
    NumericRangeGroupRep numericRangeGroupRep_;
};

NS_FACETED_END

#endif

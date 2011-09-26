#include "GroupRep.h"

#include "NumericGroupCounter.h"
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

GroupRep::GroupRep()
{
}

GroupRep::~GroupRep()
{
}

void GroupRep::swap(GroupRep& rep)
{
    stringGroupRep_.swap(rep.stringGroupRep_);
    numericGroupRep_.swap(rep.numericGroupRep_);
    numericRangeGroupRep_.swap(rep.numericRangeGroupRep_);
}

void GroupRep::merge(const GroupRep& other)
{
    mergeStringGroup(other);
    mergeNumericGroup(other);
    mergeNumericRangeGroup(other);
}

void GroupRep::mergeStringGroup(const GroupRep& other)
{
//  TODO
}

void GroupRep::mergeNumericGroup(const GroupRep& other)
{
    if (numericGroupRep_.empty())
    {
        numericGroupRep_ = other.numericGroupRep_;
        return;
    }

    NumericGroupRep::iterator it = numericGroupRep_.begin();
    NumericGroupRep::const_iterator oit = other.numericGroupRep_.begin();

    while (it != numericGroupRep_.end())
    {
        for (map<double, unsigned int>::const_iterator mit = oit->second.begin(); mit != oit->second.end(); ++mit)
        {
            it->second[mit->first] += mit->second;
        }
        ++it;
        ++oit;
    }
}

void GroupRep::mergeNumericRangeGroup(const GroupRep& other)
{
    if (numericRangeGroupRep_.empty())
    {
        numericRangeGroupRep_ = other.numericRangeGroupRep_;
        return;
    }

    NumericRangeGroupRep::iterator it = numericRangeGroupRep_.begin();
    NumericRangeGroupRep::const_iterator oit = other.numericRangeGroupRep_.begin();

    while (it != numericRangeGroupRep_.end())
    {
        vector<unsigned int>::iterator vit = it->second.begin();
        vector<unsigned int>::const_iterator ovit = oit->second.begin();

        while (vit != it->second.end())
        {
            *(vit++) += *(ovit++);
        }

        ++it;
        ++oit;
    }
}

void GroupRep::convertGroupRep()
{
    NumericGroupCounter::convertGroupRep(*this);
    NumericRangeGroupCounter::convertGroupRep(*this);
}

std::string GroupRep::ToString() const
{
    std::stringstream ss;
    for (StringGroupRep::const_iterator it = stringGroupRep_.begin();
        it != stringGroupRep_.end(); ++it)
    {
        for (std::list<OntologyRepItem>::const_iterator lit = it->begin();
            lit != it->end(); ++lit)
        {
            OntologyRepItem item = *lit;
            std::string str;
            item.text.convertString(str, izenelib::util::UString::UTF_8);
            ss<<(int)item.level<<","<<item.id<<","<<str<<","<<item.doc_count<<std::endl;
        }
    }

    return ss.str();
}

NS_FACETED_END

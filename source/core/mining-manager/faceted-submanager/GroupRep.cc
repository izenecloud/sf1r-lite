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

bool GroupRep::empty() const
{
    return (stringGroupRep_.empty() && numericGroupRep_.empty() && numericRangeGroupRep_.empty());
}

void GroupRep::swap(GroupRep& rep)
{
    stringGroupRep_.swap(rep.stringGroupRep_);
    numericGroupRep_.swap(rep.numericGroupRep_);
    numericRangeGroupRep_.swap(rep.numericRangeGroupRep_);
}

bool GroupRep::operator==(const GroupRep& other) const
{
    if (stringGroupRep_.size() != other.stringGroupRep_.size()
        || numericGroupRep_.size() != other.numericGroupRep_.size()
        || numericRangeGroupRep_.size() != other.numericRangeGroupRep_.size())
    {
        return false;
    }

    StringGroupRep::const_iterator lit1 = stringGroupRep_.begin();
    StringGroupRep::const_iterator lit2 = other.stringGroupRep_.begin();
    while (lit1 != stringGroupRep_.end())
    {
        if (lit1->size() != lit2->size())
            return false;

        list<OntologyRepItem>::const_iterator it1 = lit1->begin();
        list<OntologyRepItem>::const_iterator it2 = lit2->begin();
        while (it1 != lit1->end())
        {
            if (!(*it1 == *it2))
                return false;

            ++it1;
            ++it2;
        }
        ++lit1;
        ++lit2;
    }

    NumericGroupRep::const_iterator mit1 = numericGroupRep_.begin();
    NumericGroupRep::const_iterator mit2 = other.numericGroupRep_.begin();
    while (mit1 != numericGroupRep_.end())
    {
        if (mit1->first != mit2->first || mit1->second.size() != mit2->second.size())
            return false;

        map<double, unsigned int>::const_iterator it1 = mit1->second.begin();
        map<double, unsigned int>::const_iterator it2 = mit2->second.begin();
        while (it1 != mit1->second.end())
        {
            if (it1->first != it2->first || it1->second != it2->second)
                return false;

            ++it1;
            ++it2;
        }
        ++mit1;
        ++mit2;
    }

    NumericRangeGroupRep::const_iterator vit1 = numericRangeGroupRep_.begin();
    NumericRangeGroupRep::const_iterator vit2 = other.numericRangeGroupRep_.begin();
    while (vit1 != numericRangeGroupRep_.end())
    {
        if (vit1->first != vit2->first || vit1->second.size() != vit2->second.size())
            return false;

        vector<unsigned int>::const_iterator it1 = vit1->second.begin();
        vector<unsigned int>::const_iterator it2 = vit2->second.begin();
        while (it1 != vit1->second.end())
        {
            if (*it1 != *it2)
                return false;

            ++it1;
            ++it2;
        }
        ++vit1;
        ++vit2;
    }

    return true;
}

void GroupRep::merge(const GroupRep& other)
{
    mergeStringGroup(other);
    mergeNumericGroup(other);
    mergeNumericRangeGroup(other);
}

void GroupRep::mergeStringGroup(const GroupRep& other)
{
    if (other.stringGroupRep_.empty())
        return;

    if (stringGroupRep_.empty())
    {
        stringGroupRep_.swap(((GroupRep &) other).stringGroupRep_);
        return;
    }

//  TODO
}

void GroupRep::mergeNumericGroup(const GroupRep& other)
{
    if (other.numericGroupRep_.empty())
        return;

    if (numericGroupRep_.empty())
    {
        numericGroupRep_.swap(((GroupRep &) other).numericGroupRep_);
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
    if (other.numericRangeGroupRep_.empty())
        return;

    if (numericRangeGroupRep_.empty())
    {
        numericRangeGroupRep_.swap(((GroupRep &) other).numericRangeGroupRep_);
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

void GroupRep::toOntologyRepItemList()
{
    NumericGroupCounter::toOntologyRepItemList(*this);
    NumericRangeGroupCounter::toOntologyRepItemList(*this);
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

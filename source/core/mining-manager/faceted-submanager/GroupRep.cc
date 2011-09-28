#include "GroupRep.h"

#include "NumericGroupCounter.h"
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

//using namespace std;

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
    while (lit1 != stringGroupRep_.end() && lit2 != other.stringGroupRep_.end())
    {
        if (!(*lit1 == *lit2))
            return false;

        ++lit1;
        ++lit2;
    }

    NumericGroupRep::const_iterator mit1 = numericGroupRep_.begin();
    NumericGroupRep::const_iterator mit2 = other.numericGroupRep_.begin();
    while (mit1 != numericGroupRep_.end())
    {
        if (mit1->first != mit2->first || mit1->second.size() != mit2->second.size())
            return false;

        list<pair<double, unsigned int> >::const_iterator it1 = mit1->second.begin();
        list<pair<double, unsigned int> >::const_iterator it2 = mit2->second.begin();
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

    StringGroupRep::iterator it = stringGroupRep_.begin();
    StringGroupRep::const_iterator oit = other.stringGroupRep_.begin();

    recurseStringGroup(other, it, oit, 0);
}

void GroupRep::recurseStringGroup(
    const GroupRep& other,
    StringGroupRep::iterator &it,
    StringGroupRep::const_iterator &oit,
    uint8_t level
)
{
    while (it != stringGroupRep_.end() && it->level >= level
        && oit != other.stringGroupRep_.end() && oit->level >= level)
    {
        int comp = it->text.compare(oit->text);

        if (comp < 0)
        {
            while (++it != stringGroupRep_.end() && it->level > level);
            continue;
        }
        if (comp > 0)
        {
            stringGroupRep_.insert(it, *(oit++));
            while (oit != other.stringGroupRep_.end() && oit->level > level)
            {
                stringGroupRep_.insert(it, *(oit++));
            }
            continue;
        }
        (it++)->doc_count += (oit++)->doc_count;
        recurseStringGroup(other, it, oit, level + 1);
    }
    if (it != stringGroupRep_.end() && it->level >= level)
    {
        while (++it != stringGroupRep_.end() && it->level >= level);
    }
    else if (oit != other.stringGroupRep_.end() && oit->level >= level)
    {
        stringGroupRep_.insert(it, *(oit++));
        while (oit != other.stringGroupRep_.end() && oit->level >= level)
        {
            stringGroupRep_.insert(it, *(oit++));
        }
    }
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
        list<pair<double, unsigned int> >::iterator lit = it->second.begin();
        list<pair<double, unsigned int> >::const_iterator olit = oit->second.begin();

        while (true)
        {
            if (olit == oit->second.end()) 
                break;

            while (lit != it->second.end() && lit->first < olit->first)
            {
                ++lit;
            }

            if (lit == it->second.end())
            {
                it->second.insert(lit, olit, oit->second.end());
                break;
            }

            while (olit != oit->second.end() && lit->first > olit->first)
            {
                it->second.insert(lit, *(olit++));
            }

            while (lit != it->second.end() && olit != oit->second.end() && lit->first == olit->first)
            {
                (lit++)->second += (olit++)->second;
            }
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

string GroupRep::ToString() const
{
    stringstream ss;
    for (StringGroupRep::const_iterator it = stringGroupRep_.begin();
        it != stringGroupRep_.end(); ++it)
    {
        OntologyRepItem item = *it;
        string str;
        item.text.convertString(str, izenelib::util::UString::UTF_8);
        ss<<(int)item.level<<","<<item.id<<","<<str<<","<<item.doc_count<<endl;
    }

    return ss.str();
}

NS_FACETED_END

#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include "NumericGroupCounter.h"

NS_FACETED_BEGIN

NumericGroupCounter::NumericGroupCounter(const NumericPropertyTable *propertyTable)
    : propertyTable_(propertyTable)
{}

NumericGroupCounter::~NumericGroupCounter()
{
    delete propertyTable_;
}

void NumericGroupCounter::addDoc(docid_t doc)
{
    double value;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        ++countTable_[value];
    }
}

void NumericGroupCounter::getGroupRep(GroupRep &groupRep)
{
    groupRep.numericGroupRep_.push_back(make_pair(propertyTable_->getPropertyName(), std::list<std::pair<double, unsigned int> >()));
    std::list<std::pair<double, unsigned int> > &countList = groupRep.numericGroupRep_.back().second;

    for (std::map<double, unsigned int>::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        countList.push_back(*it);
    }
}

void NumericGroupCounter::toOntologyRepItemList(GroupRep &groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;

    for (GroupRep::NumericGroupRep::const_iterator it = groupRep.numericGroupRep_.begin();
        it != groupRep.numericGroupRep_.end(); ++it)
    {

        izenelib::util::UString propName(it->first, UString::UTF_8);
        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, 0));
        faceted::OntologyRepItem& topItem = itemList.back();
        unsigned int count = 0;

        for (std::list<std::pair<double, unsigned int> >::const_iterator lit = it->second.begin();
            lit != it->second.end(); ++lit)
        {
            std::stringstream ss;
            ss << fixed << setprecision(2);
            ss << lit->first;
            izenelib::util::UString ustr(ss.str(), UString::UTF_8);
            itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, lit->second));
            count += lit->second;
        }
        topItem.doc_count = count;
    }
    groupRep.numericGroupRep_.clear();
}

NS_FACETED_END

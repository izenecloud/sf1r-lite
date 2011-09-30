#include "NumericGroupCounter.h"

#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include "GroupRep.h"

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

NS_FACETED_END

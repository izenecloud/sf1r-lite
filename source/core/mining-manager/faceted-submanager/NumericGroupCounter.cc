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

void NumericGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, 0));
    faceted::OntologyRepItem& topItem = itemList.back();
    unsigned int count = 0;

    for (std::map<double, unsigned int>::const_iterator it = countTable_.begin();
        it != countTable_.end(); it++)
    {
        itemList.push_back(faceted::OntologyRepItem());
        faceted::OntologyRepItem& repItem = itemList.back();
        repItem.level = 1;
        std::stringstream ss;
        ss << fixed << setprecision(2);
        ss << it->first;
        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = it->second;
        count += repItem.doc_count;
    }
    topItem.doc_count = count;
}

NS_FACETED_END

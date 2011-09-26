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
    groupRep.numericGroupRep_.push_back(make_pair(propertyTable_->getPropertyName(), std::map<double, unsigned int>()));
    countTable_.swap(groupRep.numericGroupRep_.back().second);
}

void NumericGroupCounter::convertGroupRep(GroupRep &groupRep)
{
    for (GroupRep::NumericGroupRep::const_iterator it = groupRep.numericGroupRep_.begin();
        it != groupRep.numericGroupRep_.end(); ++it)
    {
        groupRep.stringGroupRep_.push_back(std::list<OntologyRepItem>());
        std::list<OntologyRepItem>& itemList = groupRep.stringGroupRep_.back();

        izenelib::util::UString propName(it->first, UString::UTF_8);
        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, 0));
        faceted::OntologyRepItem& topItem = itemList.back();
        unsigned int count = 0;

        for (std::map<double, unsigned int>::const_iterator mit = it->second.begin();
            mit != it->second.end(); mit++)
        {
            itemList.push_back(faceted::OntologyRepItem());
            faceted::OntologyRepItem& repItem = itemList.back();
            repItem.level = 1;
            std::stringstream ss;
            ss << fixed << setprecision(2);
            ss << mit->first;
            izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
            repItem.text = stringValue;
            repItem.doc_count = mit->second;
            count += repItem.doc_count;
        }
        topItem.doc_count = count;
    }
    groupRep.numericGroupRep_.clear();
}

NS_FACETED_END

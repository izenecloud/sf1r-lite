#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

DecimalSegmentTreeNode::DecimalSegmentTreeNode(int start, size_t span, DecimalSegmentTreeNode* parent)
    : start_(start)
    , span_(span)
    , count_(0)
    , parent_(parent)
{
    for (size_t i = 0; i < 10; i++)
        children_[i] = NULL;
}

DecimalSegmentTreeNode::~DecimalSegmentTreeNode()
{
    clear();
}

void DecimalSegmentTreeNode::clear()
{
    for (size_t i = 0; i < 10; i++)
    {
        if (children_[i])
        {
            children_[i]->clear();
            delete children_[i];
        }
    }
}

int DecimalSegmentTreeNode::getStart() const
{
    return start_;
}

size_t DecimalSegmentTreeNode::getSpan() const
{
    return span_;
}

size_t DecimalSegmentTreeNode::getCount() const
{
    return count_;
}

void DecimalSegmentTreeNode::insertPoint(unsigned char *digits, size_t level)
{
    count_++;
    if (level > 1)
    {
        if (!children_[*digits])
        {
            children_[*digits] = new DecimalSegmentTreeNode(start_ + (*digits) * span_ / 10, span_ /10, this);
        }
        children_[*digits]->insertPoint(digits + 1, level - 1);
    }
}

NumericRangeGroupCounter::NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, size_t rangeCount)
    : propertyTable_(propertyTable)
    , rangeCount_(rangeCount)
    , decimalSegmentTree_(0, 1000000000)
{}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
    delete propertyTable_;
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int64_t value;
    propertyTable_->getPropertyValue(doc, value);
    unsigned char *digits = new unsigned char[9]();
    for (int i = 8; i >= 0; i--)
    {
        digits[i] = value % 10;
        value /= 10;
    }
    decimalSegmentTree_.insertPoint(digits, 9);
    delete digits;
}

void NumericRangeGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    size_t count = decimalSegmentTree_.getCount();
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count));

//  TODO: design a strategy to split into several ranges

}

NS_FACETED_END

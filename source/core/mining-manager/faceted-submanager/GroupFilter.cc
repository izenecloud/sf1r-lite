#include "GroupFilter.h"
#include "GroupParam.h"
#include "group_manager.h"
#include "attr_table.h"
#include "ontology_rep.h"
#include "GroupCounter.h"
#include "AttrCounter.h"
#include "GroupLabel.h"
#include "AttrLabel.h"
#include "GroupCounterLabelBuilder.h"
#include <util/ClockTimer.h>

#include <glog/logging.h>

NS_FACETED_BEGIN

GroupFilter::GroupFilter(const GroupParam& groupParam)
    : groupParam_(groupParam)
    , attrCounter_(NULL)
{
}

GroupFilter::~GroupFilter()
{
    for (std::size_t i = 0; i < groupLabels_.size(); ++i)
    {
        delete groupLabels_[i];
    }
    groupLabels_.clear();

    for (std::size_t i = 0; i < groupCounters_.size(); ++i)
    {
        delete groupCounters_[i];
    }
    groupCounters_.clear();

    for (std::size_t i = 0; i < attrLabels_.size(); ++i)
    {
        delete attrLabels_[i];
    }
    attrLabels_.clear();

    delete attrCounter_;
}

bool GroupFilter::initGroup(const GroupManager* groupManager)
{
    GroupCounterLabelBuilder builder(groupManager);

    // map from prop name to GroupCounter instance
    typedef std::map<std::string, GroupCounter*> GroupCounterMap;
    GroupCounterMap groupCounterMap;

    const std::vector<std::string>& groupProps = groupParam_.groupProps_;
    for (std::vector<std::string>::const_iterator it = groupProps.begin();
        it != groupProps.end(); ++it)
    {
        if (groupCounterMap.find(*it) == groupCounterMap.end())
        {
            GroupCounter* counter = builder.createGroupCounter(*it);
            if (counter)
            {
                groupCounterMap[*it] = counter;
                groupCounters_.push_back(counter);
            }
        }
        else
        {
            LOG(WARNING) << "ignore duplicated group property " << *it;
        }
    }

    const GroupParam::GroupLabelVec& labels = groupParam_.groupLabels_;
    for (GroupParam::GroupLabelVec::const_iterator it = labels.begin();
        it != labels.end(); ++it)
    {
        GroupCounterMap::iterator counterIt = groupCounterMap.find(it->first);
        if (counterIt != groupCounterMap.end())
        {
            GroupCounter* counter = counterIt->second;
            GroupLabel* label = builder.createGroupLabel(*it);
            if (label)
            {
                label->setCounter(counter);
                groupLabels_.push_back(label);
            }
        }
        else
        {
            LOG(ERROR) << "not found group property " << it->first << " for group label";
            return false;
        }
    }

    return true;
}

bool GroupFilter::initAttr(const AttrTable* attrTable)
{
    attrCounter_ = new AttrCounter(attrTable);

    const GroupParam::AttrLabelVec& labels = groupParam_.attrLabels_;
    for (GroupParam::AttrLabelVec::const_iterator it = labels.begin();
        it != labels.end(); ++it)
    {
        attrLabels_.push_back(new AttrLabel(*it, attrTable));
    }

    return true;
}

bool GroupFilter::test(docid_t doc)
{
    bool result = true;
    int failIndex = 0; // the failed label index in groupLabels_ or attrLabels_

    for (std::size_t i = 0; i < groupLabels_.size(); ++i)
    {
        if (groupLabels_[i]->test(doc) == false)
        {
            // the 2nd fail
            if (result == false)
            {
                return false;
            }

            // the 1st fail
            result = false;
            failIndex = i;
        }
    }

    const bool groupResult = result;
    for (std::size_t i = 0; i < attrLabels_.size(); ++i)
    {
        if (attrLabels_[i]->test(doc) == false)
        {
            // the 2nd fail
            if (result == false)
            {
                return false;
            }

            // the 1st fail
            result = false;
            failIndex = i;
        }
    }

    if (result)
    {
        for (std::vector<GroupCounter*>::iterator it = groupCounters_.begin(); 
            it != groupCounters_.end(); ++it)
        {
            (*it)->addDoc(doc);
        }

        if (attrCounter_)
        {
            attrCounter_->addDoc(doc);
        }
    }
    else
    {
        if (groupResult)
        {
            // fail in attr label
            AttrTable::nid_t nId = attrLabels_[failIndex]->attrNameId();
            attrCounter_->addAttrDoc(nId, doc);
        }
        else
        {
            // fail in group label
            GroupCounter* counter = groupLabels_[failIndex]->getCounter();
            counter->addDoc(doc);
        }
    }

    return result;
}

void GroupFilter::getGroupRep(
    OntologyRep& groupRep,
    OntologyRep& attrRep
)
{
    izenelib::util::ClockTimer timer;

    for (std::vector<GroupCounter*>::iterator it = groupCounters_.begin(); 
        it != groupCounters_.end(); ++it)
    {
        (*it)->getGroupRep(groupRep);
    }

    if (attrCounter_)
    {
        attrCounter_->getGroupRep(groupParam_.attrGroupNum_, attrRep);
    }

    LOG(INFO) << "GroupFilter::getGroupRep() costs " << timer.elapsed() << " seconds";
}

NS_FACETED_END

#include "GroupFilter.h"
#include "GroupParam.h"
#include "GroupManager.h"
#include "GroupCounter.h"
#include "GroupLabel.h"
#include "GroupCounterLabelBuilder.h"
#include "GroupRep.h"
#include "../faceted-submanager/ontology_rep.h"
#include "../attr-manager/AttrTable.h"
#include "../attr-manager/AttrCounter.h"
#include "../attr-manager/AttrLabel.h"
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

bool GroupFilter::initGroup(GroupCounterLabelBuilder& builder)
{
    // map from prop name to GroupCounter instance
    typedef std::map<std::string, GroupCounter*> GroupCounterMap;
    GroupCounterMap groupCounterMap;

    const std::vector<GroupPropParam>& groupProps = groupParam_.groupProps_;
    for (std::vector<GroupPropParam>::const_iterator it = groupProps.begin();
        it != groupProps.end(); ++it)
    {
        const std::string& propName = it->property_;
        if (groupCounterMap.find(propName) == groupCounterMap.end())
        {
            GroupCounter* counter = builder.createGroupCounter(*it);
            if (counter)
            {
                groupCounterMap[propName] = counter;
                groupCounters_.push_back(counter);
            }
            else
            {
                LOG(ERROR) << "fail to create group counter for property " << propName;
                return false;
            }
        }
        else
        {
            LOG(WARNING) << "ignore duplicated group property " << propName;
        }
    }

    const GroupParam::GroupLabelMap& labels = groupParam_.groupLabels_;
    for (GroupParam::GroupLabelMap::const_iterator labelIt = labels.begin();
        labelIt != labels.end(); ++labelIt)
    {
        const std::string& propName = labelIt->first;
        GroupLabel* label = builder.createGroupLabel(*labelIt);
        if (label)
        {
            GroupCounterMap::iterator counterIt = groupCounterMap.find(propName);
            if (counterIt != groupCounterMap.end())
            {
                label->setCounter(counterIt->second);
            }

            groupLabels_.push_back(label);
        }
        else
        {
            LOG(ERROR) << "fail to create group label for property " << propName;
            return false;
        }
    }

    return true;
}

bool GroupFilter::initAttr(const AttrTable& attrTable)
{
    if (groupParam_.isAttrGroup_)
    {
        attrCounter_ = new AttrCounter(attrTable);
    }

    const GroupParam::AttrLabelMap& labels = groupParam_.attrLabels_;
    for (GroupParam::AttrLabelMap::const_iterator labelIt = labels.begin();
        labelIt != labels.end(); ++labelIt)
    {
        attrLabels_.push_back(new AttrLabel(attrTable,
            labelIt->first, labelIt->second));
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
            if (attrCounter_)
            {
                AttrTable::nid_t nId = attrLabels_[failIndex]->attrNameId();
                attrCounter_->addAttrDoc(nId, doc);
            }
        }
        else
        {
            // fail in group label
            GroupCounter* counter = groupLabels_[failIndex]->getCounter();
            if (counter)
            {
                counter->addDoc(doc);
            }
        }
    }

    return result;
}

void GroupFilter::getGroupRep(
    GroupRep& groupRep,
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

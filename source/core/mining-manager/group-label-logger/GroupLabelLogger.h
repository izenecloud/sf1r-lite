///
/// @file GroupLabelLogger.h
/// @brief log the clicks on group labels
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-25
///

#ifndef SF1R_GROUP_LABEL_LOGGER_H
#define SF1R_GROUP_LABEL_LOGGER_H

#include "LabelCounter.h"
#include <common/SDBWrapper.h>

#include <vector>
#include <string>

namespace sf1r
{

class GroupLabelLogger
{
public:
    typedef LabelCounter::LabelId LabelId;

    /**
     * @brief Constructor
     * @param dirPath the directory path
     * @param propName the property name
     */
    GroupLabelLogger(
        const std::string& dirPath,
        const std::string& propName
    );

    void flush();

    /**
     * Log the group label click.
     * @param query user query
     * @param labelId the id of the group label
     * @return true for success, false for failure
     */
    bool logLabel(
        const std::string& query,
        LabelId labelId
    );

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param limit the max number of labels to get
     * @param labelIdVec store the ids of each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqLabel(
        const std::string& query,
        int limit,
        std::vector<LabelId>& labelIdVec,
        std::vector<int>& freqVec
    );

    /**
     * Set the most frequently clicked group labels.
     * @param query user query
     * @param labelIdVec the ids of each group label
     * @return true for success, false for failure
     */
    bool setTopLabel(
        const std::string& query,
        const std::vector<LabelId>& labelIdVec
    );

private:
    /** query => LabelCounter */
    typedef SDBWrapper<std::string, LabelCounter> DBType;

    DBType db_;
};

}

#endif 

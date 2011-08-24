///
/// @file GroupLabelLogger.h
/// @brief log the clicks on group labels
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-25
///

#ifndef SF1R_GROUP_LABEL_LOGGER_H
#define SF1R_GROUP_LABEL_LOGGER_H

#include "LabelCounter.h"
#include <sdb/SequentialDB.h>

#include <vector>
#include <string>

namespace sf1r
{

class GroupLabelLogger {
public:
    /**
     * @brief Constructor
     * @param dirPath the directory path
     * @param propName the property name
     */
    GroupLabelLogger(
        const std::string& dirPath,
        const std::string& propName
    );

    ~GroupLabelLogger();

    void flush();

    /**
     * @brief Open the log data.
     * @return true for success, false for failure
     */
    bool open();

    /**
     * Log the group label click.
     * @param query user query
     * @param value the value of the group label
     * @return true for success, false for failure
     */
    bool logLabel(
        const std::string& query,
        LabelCounter::value_type value
    );

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param limit the max number of labels to get
     * @param valueVec store the values of each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqLabel(
        const std::string& query,
        int limit,
        std::vector<LabelCounter::value_type>& valueVec,
        std::vector<int>& freqVec
    );

    /**
     * Set the most frequently clicked group label.
     * @param query user query
     * @param value the value of group label
     * @return true for success, false for failure
     */
    bool setTopLabel(
        const std::string& query,
        LabelCounter::value_type value
    );

private:
    /** query => LabelCounter */
    typedef izenelib::sdb::unordered_sdb_tc<std::string, LabelCounter, ReadWriteLock> DBType;
    DBType container_;
};

}

#endif 

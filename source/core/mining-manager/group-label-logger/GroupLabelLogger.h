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
     * @param propValue the property value of the group label
     * @return true for success, false for failure
     */
    bool logLabel(
        const std::string& query,
        const std::string& propValue
    );

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param limit the max number of labels to get
     * @param propValueVec store the property values of each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqLabel(
        const std::string& query,
        int limit,
        std::vector<std::string>& propValueVec,
        std::vector<int>& freqVec
    );

private:
    std::string dirPath_;

    typedef izenelib::sdb::unordered_sdb_tc<std::string, LabelCounter, ReadWriteLock> DBType;
    DBType container_;
};

}

#endif 

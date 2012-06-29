#include "GroupLabelLogger.h"

#include <algorithm> // min
#include <glog/logging.h>

namespace sf1r
{

GroupLabelLogger::GroupLabelLogger(
    const std::string& dirPath,
    const std::string& propName
)
    : db_(dirPath + "/" + propName + ".db")
{
}

void GroupLabelLogger::flush()
{
    db_.flush();
}

bool GroupLabelLogger::logLabel(
    const std::string& query,
    LabelId labelId
)
{
    LabelCounter labelCounter;
    if (! db_.get(query, labelCounter))
        return false;

    labelCounter.freqCounter_.click(labelId);
    return db_.update(query, labelCounter);
}

bool GroupLabelLogger::getFreqLabel(
    const std::string& query,
    int limit,
    std::vector<LabelId>& labelIdVec,
    std::vector<int>& freqVec
)
{
    if (limit <= 0)
        return true;

    LabelCounter labelCounter;
    if (! db_.get(query, labelCounter))
        return false;

    if (labelCounter.setLabelIds_.empty())
    {
        labelCounter.freqCounter_.getFreqClick(limit, labelIdVec, freqVec);
    }
    else
    {
        std::vector<LabelId>::const_iterator beginIt =
            labelCounter.setLabelIds_.begin();

        int setIdNum = labelCounter.setLabelIds_.size();
        limit = std::min(limit, setIdNum);

        labelIdVec.assign(beginIt, beginIt + limit);
        freqVec.assign(limit, 0);
    }

    return true;
}

bool GroupLabelLogger::setTopLabel(
    const std::string& query,
    const std::vector<LabelId>& labelIdVec
)
{
    LabelCounter labelCounter;
    if (! db_.get(query, labelCounter))
        return false;

    labelCounter.setLabelIds_ = labelIdVec;
    return db_.update(query, labelCounter);
}

} // namespace sf1r

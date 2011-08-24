#include "GroupLabelLogger.h"

#include <glog/logging.h>

namespace sf1r
{

GroupLabelLogger::GroupLabelLogger(
    const std::string& dirPath,
    const std::string& propName
)
    : container_(dirPath + "/" + propName + ".db")
{
}

GroupLabelLogger::~GroupLabelLogger()
{
    flush();
}

void GroupLabelLogger::flush()
{
    try
    {
        container_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool GroupLabelLogger::open()
{
    return container_.open();
}

bool GroupLabelLogger::logLabel(
    const std::string& query,
    LabelCounter::value_type value
)
{
    LabelCounter labelCounter;
    container_.getValue(query, labelCounter);
    labelCounter.increment(value);

    bool result = false;
    try
    {
        result = container_.update(query, labelCounter);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

bool GroupLabelLogger::getFreqLabel(
    const std::string& query,
    int limit,
    std::vector<LabelCounter::value_type>& valueVec,
    std::vector<int>& freqVec
)
{
    LabelCounter labelCounter;
    container_.getValue(query, labelCounter);

    labelCounter.getFreqLabel(limit, valueVec, freqVec);

    if (valueVec.size() != freqVec.size())
        return false;

    return true;
}

bool GroupLabelLogger::setTopLabel(
    const std::string& query,
    LabelCounter::value_type value
)
{
    LabelCounter labelCounter;
    container_.getValue(query, labelCounter);
    labelCounter.setTopLabel(value);

    bool result = false;
    try
    {
        result = container_.update(query, labelCounter);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

}

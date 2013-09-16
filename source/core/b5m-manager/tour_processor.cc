#include "tour_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "scd_doc_processor.h"
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <document-manager/ScdDocument.h>

using namespace sf1r;

TourProcessor::TourProcessor()
{
}

bool TourProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance)
{
    ScdDocProcessor::ProcessorType p = boost::bind(&TourProcessor::Insert_, this, _1);
    int thread_num = 1;
    ScdDocProcessor sd_processor(p, thread_num);
    sd_processor.AddInput(scd_path);
    //sd_processor.SetOutput(writer);
    sd_processor.Process();
    return true;
}

void TourProcessor::Insert_(ScdDocument& doc)
{
    BufferValueItem value;
    doc.getString("FromCity", value.from);
    doc.getString("ToCity", value.to);
    std::string sdays;
    doc.getString("TimePlan", sdays);
    uint32_t days = ParseDays_(sdays);
    value.days = days;
    BufferKey key(value.from, value.to);
    buffer_[key].push_back(value);
}
uint32_t TourProcessor::ParseDays_(const std::string& sdays) const
{
    //TODO
    return 0;
}

void TourProcessor::Finish_()
{
    for(Buffer::iterator it = buffer_.begin();it!=buffer_.end();++it)
    {
        const BufferKey& key = it->first;
        BufferValue& value = it->second;
        std::sort(value.begin(), value.end());
    }
}



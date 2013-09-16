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
    m_ = mdb_instance;
    ScdDocProcessor::ProcessorType p = boost::bind(&TourProcessor::Insert_, this, _1);
    int thread_num = 1;
    ScdDocProcessor sd_processor(p, thread_num);
    sd_processor.AddInput(scd_path);
    //sd_processor.SetOutput(writer);
    sd_processor.Process();
    Finish_();
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
    value.doc = doc;
    std::string sprice;
    doc.getString("SalesPrice", sprice);
    value.price = 0.0;
    try {
        value.price = boost::lexical_cast<double>(sprice);
    }
    catch(std::exception& ex)
    {
        std::cerr<<"parse tour price error for "<<sprice<<std::endl;
    }
    value.bcluster = true;
    if(value.days==0||value.price==0.0||value.from.empty()||value.to.empty()) value.bcluster = false;
    BufferKey key(value.from, value.to);
    boost::unique_lock<boost::mutex> lock(mutex_);
    buffer_[key].push_back(value);
}
uint32_t TourProcessor::ParseDays_(const std::string& sdays) const
{
    //TODO
    return 0;
}

void TourProcessor::Finish_()
{
    std::string odir = m_+"/b5mo";
    std::string pdir = m_+"/b5mp";
    boost::filesystem::create_directories(odir);
    boost::filesystem::create_directories(pdir);
    ScdWriter owriter(odir, UPDATE_SCD);
    ScdWriter pwriter(pdir, UPDATE_SCD);
    for(Buffer::iterator it = buffer_.begin();it!=buffer_.end();++it)
    {
        const BufferKey& key = it->first;
        BufferValue& value = it->second;
        std::sort(value.begin(), value.end());
        std::vector<Group> groups;
        for(uint32_t i=0;i<value.size();i++)
        {
            const BufferValueItem& vi = value[i];
            Group* find_group = NULL;
            for(uint32_t j=0;j<groups.size();j++)
            {
                Group& g = groups[j];
                if(!g.front().bcluster) continue;
                //compare vi with g;
                uint32_t g_mindays = g.front().days;
                if((double)vi.days/g_mindays>1.5) continue;
                double avg_price = 0.0;
                for(uint32_t k=0;k<g.size();k++)
                {
                    avg_price += g[k].price;
                }
                avg_price/=g.size();
                double p_ratio = std::max(vi.price, avg_price)/std::min(vi.price, avg_price);
                if(p_ratio>1.5) continue;
                find_group = &g;
            }

            if(find_group==NULL)
            {
                Group g;
                g.push_back(vi);
                groups.push_back(g);
            }
            else
            {
                find_group->push_back(vi);
            }
        }
        for(uint32_t i=0;i<groups.size();i++)
        {
            Group& g = groups[i];
            std::string pid;
            g.front().doc.getString("DOCID", pid);
            for(uint32_t j=0;j<g.size();j++)
            {
                BufferValueItem& vi = g[j];
                vi.doc.property("uuid") = str_to_propstr(pid);
                owriter.Append(vi.doc);
            }
            Document pdoc;
            GenP_(g, pdoc);
            pwriter.Append(pdoc);
        }
    }
    owriter.Close();
    pwriter.Close();
}

void TourProcessor::GenP_(Group& g, Document& doc) const
{
    //TODO
}


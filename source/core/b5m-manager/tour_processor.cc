#include "tour_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "scd_doc_processor.h"
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <document-manager/ScdDocument.h>
#include <assert.h>

using namespace sf1r;

TourProcessor::TourProcessor()
{
}

TourProcessor::~TourProcessor()
{
}

bool TourProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance)
{
	//scan all the *.scd
    m_ = mdb_instance;
    ScdDocProcessor::ProcessorType p = boost::bind(&TourProcessor::Insert_, this, _1);
    int thread_num = 1;
    ScdDocProcessor sd_processor(p, thread_num);
    sd_processor.AddInput(scd_path);
    sd_processor.Process();
	return true;
}

void TourProcessor::Insert_(ScdDocument& doc)
{
	std::string sdays;
	std::string price;
	BufferValueItem value;
    doc.getString(SCD_FROM_CITY, value.from);
    doc.getString(SCD_TO_CITY, value.to);
	doc.getString(SCD_PRICE,price);
    doc.getString(SCD_TIME_PLAN, sdays);
    uint32_t days = ParseDays_(sdays);
    value.days = days;
    value.doc = doc;
    value.price = 0.0;
    try {
        value.price = boost::lexical_cast<double>(price);
    }
    catch(std::exception& ex)
    {
        std::cerr<<"parse tour price error for "<< price<<std::endl;
    }
    value.bcluster = true;
    if(value.days==0||value.price==0.0||value.from.empty()||value.to.empty()) value.bcluster = false;
    BufferKey key(value.from, value.to);
    boost::unique_lock<boost::mutex> lock(mutex_);
    buffer_[key].push_back(value);
}

uint32_t TourProcessor::ParseDays_(const std::string& sdays) const
{
    //find "天" Ps:4-7天/8天
	size_t pos = sdays.find("天");
	if(pos == std::string::npos)
	{
		pos = sdays.length();
	}
	
	std::string tmp_days = sdays.substr(0,pos);
	//find '-'
	pos = tmp_days.find('-');
	if(pos == std::string::npos)
	{
		return atoi(tmp_days.c_str());
	}
	else
	{
		int min_days = atoi(tmp_days.substr(0,pos).c_str());
		int max_days = atoi(tmp_days.substr(pos+1).c_str());
		return (min_days + max_days + 1)/2;
	}
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
	assert(g.size() >= 1);
	std::string source;
	std::string price;
	std::string p_docid;
	Set		    source_set;
	double min_price = g[0].price;
	double max_price = g[0].price;
	//get first doc  docid
	g[0].doc.getString(SCD_DOC_ID,p_docid);
	//set docid default use first docid
	doc.property(SCD_DOC_ID) = p_docid;
	for(size_t i = 0; i<g.size();i++)
	{
		ScdDocument &doc_ref = g[i].doc;
		//get source list
		doc_ref.getString(SCD_SOURCE,source);
        source_set.insert(source);   
		//get price range
		min_price = std::min(min_price,g[i].price);
		max_price = std::max(max_price,g[i].price);
		//maybe some other rules TODO........
	}

	//generate p <source>
	source.clear();
	Set::iterator iter = source_set.begin();
	while(iter != source_set.end())
	{
		source += *iter;
		source += ',';
	}
	source.erase(--source.end());
	doc.property(SCD_SOURCE)=source;

	//generate p <price>
	std::stringstream ss;
	if(min_price < max_price)
	{
		ss << min_price << '-'<< max_price;
	}
	else
	{
		assert(min_price == max_price);
		ss << min_price;
	}
	ss >> price;
	doc.property(SCD_PRICE) = price;
	//may have other attribute to set property todo......
	//
	//
}

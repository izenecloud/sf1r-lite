#include "tour_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "scd_doc_processor.h"
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <document-manager/ScdDocument.h>
#include <assert.h>

//#define TOUR_DEBUG

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
    Finish_();
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
    value.days = ParseDays_(sdays);
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
    if(value.days.second==0||value.price==0.0||value.from.empty()||value.to.empty()) value.bcluster = false;
 
	//generate union key by to_city
	UnionBufferKey union_key;
	GenerateUnionKey(union_key,value.from,value.to);
	//BufferKey key(value.from, value.to);
    boost::unique_lock<boost::mutex> lock(mutex_);
    buffer_[union_key].push_back(value);
}

std::pair<uint32_t, uint32_t> TourProcessor::ParseDays_(const std::string& sdays) const
{
    //find "天" Ps:4-7天/8天
    std::pair<uint32_t, uint32_t> r(0,0);
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
		r.first = atoi(tmp_days.c_str());
		r.second = r.first;
	}
	else
	{
		int min_days = atoi(tmp_days.substr(0,pos).c_str());
		int max_days = atoi(tmp_days.substr(pos+1).c_str());
		r.first = min_days;
		r.second = max_days;
	}
	return r;
}

void TourProcessor::GenerateUnionKey(UnionBufferKey& union_key,
									 const std::string& from_city,
									 const std::string& to_city)const
{
	std::vector<std::string> union_to_city;
	boost::split(union_to_city,to_city,boost::is_any_of(","));

	if(union_to_city.size() == 0)
	{
		union_key.push_back(std::make_pair(from_city,""));
		return;
	}

	for(size_t i = 0;i<union_to_city.size();i++)
	{
		//ps:from city:上海  to city：上海，马累，马尔代夫 
		//so we should remove key pair(上海，上海)
		if(union_to_city[i].empty()) continue;
		if(union_to_city.size() >= 2)
		{
			if(from_city == union_to_city[i]) continue;
		}
		union_key.push_back(std::make_pair(from_city,union_to_city[i]));
	}

	if(union_key.size()==0)
	{
		union_key.push_back(std::make_pair(from_city,from_city));
	}

	//sort key vector
	std::sort(union_key.union_key_.begin(),union_key.union_key_.end());
}

void TourProcessor::Finish_()
{
    LOG(INFO)<<"buffer size "<<buffer_.size()<<std::endl;
    std::string odir = m_+"/b5mo";
    std::string pdir = m_+"/b5mp";
    boost::filesystem::create_directories(odir);
    boost::filesystem::create_directories(pdir);
	ScdWriter owriter(odir, UPDATE_SCD);
    ScdWriter pwriter(pdir, UPDATE_SCD);
    for(Buffer::iterator it = buffer_.begin();it!=buffer_.end();++it)
    {
        const UnionBufferKey& key = it->first;
        BufferValue& value = it->second;
#ifdef TOUR_DEBUG
		LOG(INFO) << "1---->processing:";
		for(size_t i = 0; i<key.union_key_.size();i++)
		{
			LOG(INFO) << key.union_key_[i].first << "," << key.union_key_[i].second << "	";
		}
		LOG(INFO) << "aggregate count:" << value.size();
#endif
        std::sort(value.begin(), value.end());
        std::vector<Group> groups;
        for(uint32_t i=0;i<value.size();i++)
        {
            const BufferValueItem& vi = value[i];
#ifdef TOUR_DEBUG
            LOG(INFO)<<"find value item "<<vi.from<<","<<vi.to<<",["<<vi.days.first<<","<<vi.days.second<<"],"<<vi.price<<","<<vi.bcluster<<std::endl;
#endif
            Group* find_group = NULL;
            for(uint32_t j=0;j<groups.size();j++)
            {
                Group& g = groups[j];
                if(!g.front().bcluster) continue;
                //compare vi with g;
                std::pair<uint32_t, uint32_t> g_mindays = g.front().days;
                if(vi.days.first>g_mindays.second&&vi.days.first-g_mindays.second>1) continue;
                //if(vi.days-g_mindays>1) continue;
                //double avg_price = 0.0;
                //for(uint32_t k=0;k<g.size();k++)
                //{
                //    avg_price += g[k].price;
                //}
                //avg_price/=g.size();
                //double p_ratio = std::max(vi.price, avg_price)/std::min(vi.price, avg_price);
                //if(p_ratio>1.5) continue;
                find_group = &g;
            }

            if(find_group==NULL)
            {
                Group g;
                g.push_back(vi);
                groups.push_back(g);
#ifdef TOUR_DEBUG
                LOG(INFO)<<"create new group"<<std::endl;
#endif
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
	doc = g[0].doc;
	doc.eraseProperty("uuid");
	std::string price;
	std::string p_docid;
    Set source_set;
	double min_price = g[0].price;
	double max_price = g[0].price;
	//get first doc uuid as pid
	g[0].doc.getString(SCD_UUID,p_docid);
	//set docid default use first docid
	doc.property(SCD_DOC_ID) = str_to_propstr(p_docid);
	for(std::size_t i=0;i<g.size();i++)
	{
		ScdDocument& doc_ref = g[i].doc;
		//get source list
        std::string source;
		doc_ref.getString(SCD_SOURCE,source);
		if(!source.empty()) source_set.insert(source);   
		//get price range
		min_price = std::min(min_price,g[i].price);
		max_price = std::max(max_price,g[i].price);
		//maybe some other rules TODO........
	}

	//generate p <source>
    std::string source;
	for(Set::const_iterator it = source_set.begin();it!=source_set.end();++it)
	{
	    if(!source.empty()) source+=",";
		source += *it;
	}
	doc.property(SCD_SOURCE)=str_to_propstr(source);

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
	doc.property(SCD_PRICE) = str_to_propstr(price);
	doc.property("itemcount") = (int64_t)(g.size());
	//may have other attribute to set property todo......
	//
	//
}

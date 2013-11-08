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
	Destroy();
}

void TourProcessor::Destroy()
{
	for(Buffer::iterator iter = buffer_.begin(); iter != buffer_.end(); ++iter)
	{
		BufferValue &buffer_value = iter->second;
		for(size_t i = 0; i < buffer_value.size();i++)
		{
			assert(buffer_value[i].doc);
			delete buffer_value[i].doc;
		}
	}
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

void TourProcessor::Insert_(ScdDocument& doc)
{
	std::string name;
	std::string from_city;
	std::string to_city;
	ScdDocument *duplicate_doc = new ScdDocument(doc);
	duplicate_doc->getString(SCD_NAME,name);
	duplicate_doc->getString(SCD_FROM_CITY,from_city);
	duplicate_doc->getString(SCD_TO_CITY,to_city);
	GenerateAuxiliaryHash(duplicate_doc,from_city,to_city,name);
}

void TourProcessor::GenerateGroup(DocSet& docs)
{
	assert(docs.size()>=1);
	std::string sdays;
	std::string price;
	std::string from_city;
	std::string to_city;
	//use first document <from,to> as group key
	(*docs.begin())->getString(SCD_FROM_CITY,from_city);
	(*docs.begin())->getString(SCD_TO_CITY,to_city);
	BufferKey key(from_city,to_city);

	for(DocSet::const_iterator iter = docs.begin(); iter != docs.end(); ++iter)
	{
		BufferValueItem value;
		(*iter)->getString(SCD_FROM_CITY, value.from);
		(*iter)->getString(SCD_TO_CITY, value.to);
		(*iter)->getString(SCD_PRICE,price);
		(*iter)->getString(SCD_TIME_PLAN, sdays);
		value.days = ParseDays_(sdays);
		value.doc = static_cast<ScdDocument*>(*iter);
		assert(value.doc);
		value.price = 0.0;
		try {
			value.price = boost::lexical_cast<double>(price);
		}
		catch(std::exception& e)
		{
			LOG(INFO) << e.what();
		}
		value.bcluster = true;
		if(value.days.second==0||value.price==0.0||
					value.from.empty()||value.to.empty())
		{
			value.bcluster = false;
		}
		buffer_[key].push_back(value);
	}
}

void TourProcessor::GenerateAuxiliaryHash(Document*doc,
										  const std::string&from_city,
										  const std::string&to_city,
										  const std::string&name)
{
	assert(doc);
	bool is_match_city = false;
	Set to_city_split_result;
	to_city_split_result.clear();

	boost::split(to_city_split_result,to_city,boost::is_any_of(","));

	//match to_city with name property
	for(Set::const_iterator iter = to_city_split_result.begin();
				iter != to_city_split_result.end(); ++iter)
	{
		if(to_city_split_result.size() >= 2)
		{
			if(from_city == *iter) continue;
		}
		
		if(name.find(*iter) == std::string::npos)
		{
			continue;
		}

		is_match_city = true;
		BufferKey from_to_city(from_city,*iter);
		doc_hash_[doc].push_back(from_to_city);
		from_to_hash_[from_to_city].push_back(doc);
	}

	if(is_match_city == false)
	{
		//if no match to name, use all the possible to city split result
		for(Set::const_iterator iter = to_city_split_result.begin();
					iter != to_city_split_result.end(); ++iter)
		{
			if(to_city_split_result.size() >= 2)
			{
				if(from_city == *iter) continue;
			}

			BufferKey from_to_city(from_city,*iter);
			doc_hash_[doc].push_back(from_to_city);
			from_to_hash_[from_to_city].push_back(doc);
		}
	}
}

void TourProcessor::SearchGroupDocuments(DocSet& already_used_docs,
										 FromToCitySet& already_used_from_to,
										 Document *doc
										 )
{	
	if(doc == NULL) return;
	FromToCityVector &from_to_citys = doc_hash_[doc];
	assert(from_to_citys.size() >= 1);
	FromToHash::iterator iter = from_to_hash_.end();

	for(size_t idx = 0; idx < from_to_citys.size(); idx++)
	{
		if(already_used_from_to.find(from_to_citys[idx]) == already_used_from_to.end())
		{
			//insert different <from,to>
			already_used_from_to.insert(from_to_citys[idx]);
			iter = from_to_hash_.find(from_to_citys[idx]);
			assert(iter != from_to_hash_.end());
			DocVector &docs = iter->second;
			assert(docs.size()>=1);
			
			for(size_t doc_idx = 0; doc_idx < docs.size();doc_idx++)
			{
				if(already_used_docs.find(docs[doc_idx]) == already_used_docs.end())
				{
					already_used_docs.insert(docs[doc_idx]);
					//recursive search process 
					SearchGroupDocuments(already_used_docs,already_used_from_to,docs[doc_idx]);
				}
			}
		}
	}
}

void TourProcessor::LogGroup(const DocSet& already_used_docs)
{
	static int group_index = 1;
	LOG(INFO) << "1--->group " << group_index++ << " count:" << already_used_docs.size(); 
	
	for(DocSet::const_iterator doc_iter= already_used_docs.begin(); 
			doc_iter != already_used_docs.end(); doc_iter++)
	{
		ScdDocument *p = (ScdDocument*)(*doc_iter);
		std::string doc_id;
		p->getString(SCD_DOC_ID,doc_id);
		std::string from,to;
		p->getString(SCD_FROM_CITY,from);
		p->getString(SCD_TO_CITY,to);
		LOG(INFO) << "\t" << doc_id << ":" << from <<","<<to;
	}
	LOG(INFO) << "\n";
}

void TourProcessor::AggregateSimilarFromTo()
{
	DocSet			 already_used_docs;
	FromToCitySet	 already_used_from_to;

	FromToHash::const_iterator iter = from_to_hash_.begin();
	while(iter != from_to_hash_.end())
	{
		if(already_used_from_to.find(iter->first) != already_used_from_to.end())
		{
			++iter;
			continue;
		}
		
		already_used_from_to.insert(iter->first);
		const DocVector &docs = iter->second;
		for(size_t doc_idx = 0; doc_idx < docs.size(); doc_idx++)
		{
			if(already_used_docs.find(docs[doc_idx]) == already_used_docs.end())
			{
				already_used_docs.insert(docs[doc_idx]);
			}
			//find similar <from-to> documents
			SearchGroupDocuments(already_used_docs,already_used_from_to,docs[doc_idx]);		
		}

		LogGroup(already_used_docs);
		GenerateGroup(already_used_docs);
		//Notice:clear previous group documents
		already_used_docs.clear();
		++iter;
	}
}

void TourProcessor::Finish_()
{
	AggregateSimilarFromTo();
    std::string odir = m_+"/b5mo";
    std::string pdir = m_+"/b5mp";
    boost::filesystem::create_directories(odir);
    boost::filesystem::create_directories(pdir);
	ScdWriter owriter(odir, UPDATE_SCD);
    ScdWriter pwriter(pdir, UPDATE_SCD);
    for(Buffer::iterator it = buffer_.begin();it!=buffer_.end();++it)
    {
        BufferValue& value = it->second;
#ifdef TOUR_DEBUG
        const BufferKey& key = it->first;
		LOG(INFO) << "1---->aggregate <" << key.first << "," 
					<< key.second << "> count:"<< value.size();  
#endif
        std::sort(value.begin(), value.end());
        std::vector<Group> groups;
        for(uint32_t i=0;i<value.size();i++)
        {
            const BufferValueItem& vi = value[i];
#ifdef TOUR_DEBUG
            LOG(INFO)<<"1---->find value item "<<vi.from<<","<<vi.to<<",["<<vi.days.first<<","
						<<vi.days.second<<"],"<<vi.price<<","<<vi.bcluster<<std::endl;
#endif
            Group* find_group = NULL;
            for(uint32_t j=0;j<groups.size();j++)
            {
                Group& g = groups[j];
                if(!g.front().bcluster) continue;
                //compare vi with g;
                std::pair<uint32_t, uint32_t> g_mindays = g.front().days;
                if(vi.days.first > g_mindays.second && 
						vi.days.first - g_mindays.second > 1)
				{
					continue;
				}
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
                LOG(INFO)<<"1---->create new group"<<std::endl;
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
            g.front().doc->getString(SCD_DOC_ID, pid);
            for(uint32_t j=0;j<g.size();j++)
            {
                BufferValueItem& vi = g[j];
                vi.doc->property(SCD_UUID) = str_to_propstr(pid);
                owriter.Append(*vi.doc);
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
	doc =*g[0].doc;
	doc.eraseProperty(SCD_UUID);
	std::string price;
	std::string p_docid;
    Set source_set;
	double min_price = g[0].price;
	double max_price = g[0].price;
	//get first doc uuid as pid
	g[0].doc->getString(SCD_UUID,p_docid);
	//set docid default use first docid
	doc.property(SCD_DOC_ID) = str_to_propstr(p_docid);
	for(std::size_t i=0;i<g.size();i++)
	{
		ScdDocument& doc_ref = *g[i].doc;
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
}

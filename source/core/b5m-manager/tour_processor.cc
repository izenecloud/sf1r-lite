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
	//destroy all docs
	for(DocHash::iterator iter = doc_hash_.begin();
			iter != doc_hash_.end();iter++)
	{
		assert(iter->second);
		delete (iter->second);
	}
}

int TourProcessor::InitializeOPath(const std::string& mdb_instance)
{
    std::string b5mo_path = B5MHelper::GetB5moPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(b5mo_path);
    try
	{
		ScdWriter *o_writer = new ScdWriter(b5mo_path,UPDATE_SCD);
		pwriter_.reset(o_writer);
	}
	catch(std::exception&e)
	{
		LOG(INFO) << e.what() << std::endl;
		return 0;
	}
	return 1;
}


int TourProcessor::InitializePPath(const std::string& mdb_instance)
{
    std::string b5mp_path = B5MHelper::GetB5mpPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(b5mp_path);
    try
	{
		ScdWriter *p_writer = new ScdWriter(b5mp_path,UPDATE_SCD);
		pwriter_.reset(p_writer);
	}
	catch(std::exception&e)
	{
		LOG(INFO) << e.what() << std::endl;
		return 0;
	}
	return 1;
}

bool TourProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance)
{
	if(0 == InitializeOPath(mdb_instance))
	{
		return false;
	}

	//scan all the *.scd
    ScdDocProcessor::ProcessorType p = boost::bind(&TourProcessor::Insert_, this, _1);
    int thread_num = 1;
    ScdDocProcessor sd_processor(p, thread_num);
    sd_processor.AddInput(scd_path);
    sd_processor.Process();
	Merger();
	LOG(INFO) << "1--->O output finished." << std::endl;
	if(0 == InitializePPath(mdb_instance))
	{
		return false;
	}
	//Ouput p 
	Finish_();
	LOG(INFO) << "1--->P output finished." << std::endl;
	return true;
}

void TourProcessor::SetUUID(ScdDocument &doc)
{
	std::string doc_id;
	doc.getProperty(SCD_DOC_ID,doc_id);
	doc.property(SCD_UUID) = doc_id;  
}

void TourProcessor::Merger()
{
}

void TourProcessor::Insert_(ScdDocument& doc)
{
	//callback function aggregate the documents have the same 
	//fromcity/tocity
	//sorted by timeplan
	std::string sdays;
	std::string price;
	BufferValueItem value;
    doc.getString(SCD_FROM_CITY, value.from);
    doc.getString(SCD_TO_CITY, value.to);
	doc.getString(SCD_DOC_ID,value.docid);
	doc.getString(SCD_PRICE,price);
	value.price = atof(price.c_str());
    doc.getString(SCD_TIME_PLAN, sdays);
    uint32_t days = ParseDays_(sdays);
    value.days = days;
    BufferKey key(value.from, value.to);
    buffer_[key].push_back(value);

	//set b5mo scd <docid>------><uuid>
	SetUUID(doc);
	//write o result
	pwriter_->Append(doc);

	//generate one copy of doc prepare for the p result
	ScdDocument *copy_doc = new ScdDocument(doc);
	doc_hash_[value.docid] = copy_doc;
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

void TourProcessor::AggregateP(const BufferValue&value)
{	
	assert(value.size() >= 1);
	std::string source;
	std::string price;
	std::string p_docid = value[0].docid;
	Set		    source_set;
	ScdDocument *doc = NULL;
	double min_price = value[0].price;
	double max_price = value[0].price;
	for(size_t i = 0; i<value.size();i++)
	{
		const std::string& docid = value[i].docid;
		doc = doc_hash_[docid];
		assert(doc != NULL);
		//get source list
		doc->getString(SCD_SOURCE,source);
        source_set.insert(source);   
		//get price range
		min_price = std::min(min_price,value[i].price);
		max_price = std::max(max_price,value[i].price);
		//maybe some other rules TODO........
		//
	}


	doc = doc_hash_[p_docid];
	assert(doc != NULL);
	//generate p <source>
	source.clear();
	Set::iterator iter = source_set.begin();
	while(iter != source_set.end())
	{
		source += *iter;
		source += ',';
	}
	source.erase(--source.end());
	doc->property(SCD_SOURCE)=source;

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
	doc->property(SCD_PRICE) = price;
	pwriter_->Append(*doc);
}

void TourProcessor::Finish_()
{
    for(Buffer::iterator it = buffer_.begin();it!=buffer_.end();++it)
    {
        BufferValue& value = it->second;
        std::sort(value.begin(), value.end());
		AggregateP(value);
	}
}

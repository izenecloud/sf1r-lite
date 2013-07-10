/**
 * @file process/controllers/ExtractKeywordsController.h
 * @author Kuilong Liu
 * @date Created <2013-06-26 13:28:32>
 */
#include "ExtractKeywordsController.h"
#include <boost/lexical_cast.hpp>
#include <common/Keys.h>
#include <common/XmlConfigParser.h>
#include <list>

 namespace sf1r
 {
 	using driver::Keys;
 	using namespace izenelib::driver;
 	using izenelib::util::UString;

 	ExtractKeywordsController::ExtractKeywordsController()
 	{
 		matcher_ = ProductMatcherInstance::get();
 		if ( !(matcher_->IsOpen()))
 		{
 			std::string system_resource_path_ = SF1Config::get()->getResourceDir();
 			std::string kd = system_resource_path_ + "/product-matcher";
 			matcher_->Open(kd);
 		}
 	}

 	void ExtractKeywordsController::extract_keywords()
 	{
 		std::string page = asString(request()[Keys::text]);
 		UString uspage;
 		uspage.assign(page, izenelib::util::UString::UTF_8);
 		std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > > res_ca, res_brand, res_model;
 		std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >::iterator it;
// 		LOG(INFO)<<"content: "<<page<<endl;
 		matcher_->ExtractKeywordsFromPage(uspage, res_ca, res_brand, res_model);
 		LOG(INFO)<<"keywords count: "<<res_ca.size() + res_brand.size() + res_model.size()<<endl;

 		Value& keywords = response()[Keys::keywords];
 		for(it=res_ca.begin();it!=res_ca.end();it++)
 		{
 			Value& keyword = keywords();
 			std::string value;
 			it->first.convertString(value, izenelib::util::UString::UTF_8);
// 			LOG(INFO)<<"keyword: "<<value<<endl;
 			keyword[Keys::value] = value;
 			keyword[Keys::pos] = boost::lexical_cast<std::string>((it->second).first);
 			keyword[Keys::weight] = boost::lexical_cast<std::string>((it->second).second);
 			keyword[Keys::type] = "category";
 		}
 		for(it=res_brand.begin();it!=res_brand.end();it++)
 		{
 			Value& keyword = keywords();
 			std::string value;
 			it->first.convertString(value, izenelib::util::UString::UTF_8);
// 			LOG(INFO)<<"keyword: "<<value<<endl;
 			keyword[Keys::value] = value;
 			keyword[Keys::pos] = boost::lexical_cast<std::string>((it->second).first);
 			keyword[Keys::weight] = boost::lexical_cast<std::string>((it->second).second);
 			keyword[Keys::type] = "brand";
 		}
 		for(it=res_model.begin();it!=res_model.end();it++)
 		{
 			Value& keyword = keywords();
 			std::string value;
 			it->first.convertString(value, izenelib::util::UString::UTF_8);
// 			LOG(INFO)<<"keyword: "<<value<<endl;
 			keyword[Keys::value] = value;
 			keyword[Keys::pos] = boost::lexical_cast<std::string>((it->second).first);
 			keyword[Keys::weight] = boost::lexical_cast<std::string>((it->second).second);
 			keyword[Keys::type] = "model";
 		}
/*
 	void ExtractKeywordsController::extract_keywords()
 	{
 		std::string page = asString(request()[Keys::text]);
 		UString uspage;
 		uspage.assign(page, izenelib::util::UString::UTF_8);
 		std::map<std::string, std::vector<std::pair<UString, uint32_t> > > res;
 		std::map<UString, uint32_t> we;
 		std::list<std::pair<UString, uint32_t> > ca;
 		std::list<std::pair<UString, uint32_t> >::iterator it;
 		LOG(INFO)<<"content: "<<page<<endl;
 		matcher_->ExtractKeywordsFromPage(uspage, res, ca, we);
 		
 		Value& category = response()[Keys::category];
 		for(it=ca.begin();it!=ca.end();it++)
 		{
 			Value& c = category();
 			std::string value;
 			it->first.convertString(value, izenelib::util::UString::UTF_8);
 			LOG(INFO)<<"keyword: "<<value<<endl;
 			c[Keys::value] = value;
 			c[Keys::pos] = boost::lexical_cast<std::string>(it->second);
 		}
 		
 		Value& products = response()[Keys::products];
 		std::map<std::string, std::vector<std::pair<UString, uint32_t> > >::iterator iter;
 		for(iter=res.begin();iter!=res.end();iter++)
 		{
 			Value& product = products();
 			product[Keys::title] = iter->first;
 			Value& keywords = product()[Keys::keywords];
 			std::vector<std::pair<UString, uint32_t> >::iterator it;
 			for(it=iter->second.begin();it!=iter->second.end();it++)
 			{
 				Value& keyword = keywords();
 				std::string value;
 				it->first.convertString(value, izenelib::util::UString::UTF_8);
 				LOG(INFO)<<"keyword: "<<value<<endl;
 				keyword[Keys::value] = value;
 				keyword[Keys::pos] = boost::lexical_cast<std::string>(it->second);
 				keyword[Keys::weight] = we[it->first];
 			}
 		}
*/
 	}
 }// end of namespace sf1r


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
 		std::list<std::pair<UString, uint32_t> > res;
 		std::list<std::pair<UString, uint32_t> >::iterator it;
 		LOG(INFO)<<"content: "<<page<<endl;
 		matcher_->ExtractKeywordsFromPage(uspage, res);
 		LOG(INFO)<<"keywords count: "<<res.size()<<endl;

 		Value& keywords = response()[Keys::keywords];
 		for(it=res.begin();it!=res.end();it++)
 		{
 			Value& keyword = keywords();
 			std::string value;
 			it->first.convertString(value, izenelib::util::UString::UTF_8);
 			LOG(INFO)<<"keyword: "<<value<<endl;
 			keyword[Keys::value] = value;
 			keyword[Keys::pos] = boost::lexical_cast<std::string>(it->second);
 		}
 	}
 }// end of namespace sf1r


/**
 * \file QueryNormalizeController.cpp
 * \brief Controller QeryNormalize dictionary;
 * \date 2012-08-10
 * \author Hongliang.zhao
 */

#include "QueryNormalizeController.h"
#include <common/XmlConfigParser.h>

#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

namespace sf1r
{
/**
 * @brief Action @b add_Query. Add a Query Normalize pairs to QUERYNORMALIZE dictionary.
 *
 * @section request
 *
 * - @b queryNormalize_array:like"iphone#4s#4#3gs#".the brand'length is more than three, and we need to have at least two '#'
 * @section response
 *
 * No extra fields.
 **/

void QueryNormalizeController::add_Query()
{
    Value::StringType queryNormalize = asString(request()[Keys::NormalizeQuery]);
    if (queryNormalize.empty())
    {
        response().addError("Require queryNormalize in request.");
        return;
    }

    izenelib::util::UString query(queryNormalize, izenelib::util::UString::UTF_8);
//----------------------check-----------------
    string NormalizeQuery;
    query.convertString(NormalizeQuery, izenelib::util::UString::UTF_8);
    uint32_t len = NormalizeQuery.length();
    if(len == 0 ||len > 100)
    {
	response().addError("too long, like\"iphone#4s#4#3gs#\"");
    }
    if(NormalizeQuery[len - 1] != '#')
    {
	response().addError("the last charactor must be '#', like\"iphone#4s#4#3gs#\"");
    }
    uint32_t count = 0;
    for(uint32_t i = 0; i < len; i++)
    {
	if(NormalizeQuery[i] == '#')
	{	
	    count++;
	    if(i < 3)
		response().addError("the length of brand must longer than 3 '#', like\"iphone#4s#4#3gs#\"");
	}
    }
    if(count < 2)
	response().addError("we need at least one brand and one type, like\"iphone#4s#\"");
//----------------------load------------------
    std::string dictionaryFile = SF1Config::get()->getResourceDir();
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/qn/QUERYNORMALIZE";
    }
    else
    {
        dictionaryFile += "qn/QUERYNORMALIZE";
    }

    if(!load(dictionaryFile.c_str()))
	response().addError("the file ResourceDir/qn/QUERYNORMALIZE not exist");

    add_query(NormalizeQuery);

    save();
}


void QueryNormalizeController::delete_Query()
{
    Value::StringType queryNormalize = asString(request()[Keys::NormalizeQuery]);
    if (queryNormalize.empty())
    {
        response().addError("Require queryNormalize in request.");
        return;
    }

    izenelib::util::UString query(queryNormalize, izenelib::util::UString::UTF_8);
//----------------------check-----------------
    string NormalizeQuery;
    query.convertString(NormalizeQuery, izenelib::util::UString::UTF_8);
    uint32_t len = NormalizeQuery.length();
    if(NormalizeQuery[len - 1] != '#')
    {
	response().addError("the last charactor must be '#', like\"iphone#4s#4#3gs#\"");
    }
    uint32_t count = 0;
    for(uint32_t i = 0; i < len; i++)
    {
	if(NormalizeQuery[i] == '#')
	{	
	    count++;
	    if(i < 3)
		response().addError("the length of brand must longer than 3 '#', like\"iphone#4s#4#3gs#\"");
	}
    }
    if(count < 2)
	response().addError("we need at least one brand and one type, like\"iphone#4s#\"");
//----------------------load------------------
    std::string dictionaryFile = SF1Config::get()->getResourceDir();
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/qn/QUERYNORMALIZE";
    }
    else
    {
        dictionaryFile += "qn/QUERYNORMALIZE";
    }
    
    if(!load(dictionaryFile.c_str()))
	response().addError("the file ResourceDir/qn/QUERYNORMALIZE not exist");
    
    delete_query(NormalizeQuery);
    
    save();
}

bool QueryNormalizeController::add_query(const string &query)
{
    string brand;
    string type;
    uint32_t len = query.find_first_of('#');
    brand = query.substr(0, len);
    type = query.substr(len + 1);
    uint32_t i = 0;
    while(i < count_)
    {
        if(keyStr_[i] == brand)
	{
	    uint32_t j = 0;
	    uint32_t count = 0;
            while(j < type.length())
	    {
		if(type[j] == '#')
		    count++;
		j++;
   	    }

	    string *typeStr = new string[count];
	    j = 0;
	    count = 0;
	    uint32_t tmp = j;
	    while(j < type.length())
    	    {
		if(type[j] == '#')
		{
                    typeStr[count] = type.substr(tmp, j - tmp);//'#'	    
			tmp = j + 1;
		    count++;
		}
		j++;
   	    }
	    
            for(j = 0; j < count; j++)
	    {
		if(valueStr_[i].find((typeStr[j] + "#")) == string::npos)//if is substr ,error
		{			
		    valueStr_[i].append(typeStr[j]);
		    valueStr_[i].append("#");
		}
            }
            delete [] typeStr;
	    return true;
	}
	i++;
    }
    if(count_ < MAX_QUERYWORD_LENGTH)
    {
	keyStr_[count_] = brand;
	valueStr_[count_] = type;
    	count_++;
    }
    return true;
}

bool QueryNormalizeController::delete_query(const string &query)
{
    string brand;
    string type;
    uint32_t len = query.find_first_of('#');
    brand = query.substr(0, len);
    type = query.substr(len + 1);
    uint32_t i = 0;
    while(i < count_)
    {
        if(keyStr_[i] == brand)
	{
	    uint32_t j = 0;
	    uint32_t count = 0;
            while(j < type.length())
	    {
		if(type[j] == '#')
		    count++;
		j++;
   	    }
	    string *typeStr = new string[count];
	    j = 0;
	    count = 0;
	    uint32_t tmp = j;
	    while(j < type.length())
    	    {
		if(type[j] == '#')
		{
                    typeStr[count] = type.substr(tmp, j - tmp);//'#'
		    tmp = j + 1;
                    count++;
		}
		j++;
   	    }
            for(j = 0; j < count; j++)
	    {
		uint32_t pos = 0;
		if(valueStr_[i].find((typeStr[j] + '#')) != string::npos)
		{
			pos = valueStr_[i].find(typeStr[j] + "#");
			valueStr_[i].erase(pos, (typeStr[j].length() + 1));
		}
            }
	    return true;
	}
	i++;
    }
    return false;
}

void QueryNormalizeController::save()
{
    fstream out;
    const char* path = filePath_.c_str();
    out.open(path, ios::out);
    uint32_t i = 0;
    out<<"#format [brand#type#type#...]"<<endl;
    out<<"#rules: brand.lenth() >= 3"<<endl;
    while(i < count_)
    {
	out<<keyStr_[i]<<'#'<<valueStr_[i]<<endl;
	i++;
    }
    out.close();
    delete [] keyStr_;
    delete [] valueStr_;
}

bool QueryNormalizeController::load(string nameFile)
{
    keyStr_ = new string[MAX_QUERYWORD_LENGTH];
    valueStr_ = new string[MAX_QUERYWORD_LENGTH];
    count_ = 0;
    filePath_ = nameFile;
    fstream in;
    const char* path = filePath_.c_str();
    in.open(path, ios::in);
    if(in.good())
    {
        string strline;
        while(!in.eof())
        {
            getline(in, strline);
            if(strline[0] != '#' && strline.length() > 1)
            {
		uint32_t len = strline.find_first_of('#');
    		keyStr_[count_] = strline.substr(0, len);
    		valueStr_[count_] = strline.substr(len + 1);
    		count_++;
            }
        }
        in.close();
    }
    else
    {
	return false;
    }
    return true;
}

}// namespace  sf1r

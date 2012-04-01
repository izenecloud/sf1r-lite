#include "product_db.h"
#include <sstream>
using namespace sf1r;

ProductProperty::ProductProperty()
:itemcount(0)
{
}

ProductProperty::ProductProperty(const Document& doc)
:itemcount(1)
{
    UString usource;
    UString uprice;
    doc.getProperty("Source", usource);
    doc.getProperty("Price", uprice);
    price.Parse(uprice);
    std::string ssource;
    usource.convertString(ssource, UString::UTF_8);
    if(ssource.length()>0)
    {
        SourceMap::iterator it = source.find(ssource);
        if(it==source.end())
        {
            source.insert(std::make_pair(ssource, 1));
        }
        else
        {
            it->second += 1;
        }
    }
}

std::string ProductProperty::GetSourceString() const
{
    std::string result;
    for(SourceMap::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        if(it->second>0)
        {
            if(!result.empty()) result+=",";
            result += it->first;
        }
    }
    return result;
}

izenelib::util::UString ProductProperty::GetSourceUString() const
{
    return izenelib::util::UString(GetSourceString(), izenelib::util::UString::UTF_8);
}

ProductProperty& ProductProperty::operator+=(const ProductProperty& other)
{
    price += other.price;
    for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    {
        SourceMap::iterator it = source.find(oit->first);
        if(it==source.end())
        {
            source.insert(std::make_pair(oit->first, oit->second));
        }
        else
        {
            it->second += oit->second;
        }
    }
    itemcount+=1;
    return *this;
}

ProductProperty& ProductProperty::operator-=(const ProductProperty& other)
{
    for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    {
        SourceMap::iterator it = source.find(oit->first);
        if(it==source.end())
        {
            source.insert(std::make_pair(oit->first, -1*oit->second));
            //invalid data
            //LOG(WARNING)<<"invalid data"<<std::endl;
        }
        else
        {
            it->second -= oit->second;
            LOG(INFO)<<oit->first<<" reduced from "<<it->second+oit->second<<" to "<<it->second<<std::endl;
            //if(it->second<0)
            //{
                ////invalid data
                //LOG(WARNING)<<"invalid data"<<std::endl;
                //it->second = 0;
            //}
        }
    }
    itemcount -= 1;
    //if(itemcount>1)
    //{
        //itemcount-=1;
    //}
    //else
    //{
        ////invalid data
        //LOG(WARNING)<<"invalid data"<<std::endl;
    //}

    //TODO how to process price?

    return *this;
}


std::string ProductProperty::ToString() const
{
    std::stringstream ss;
    ss<<"itemcount:"<<itemcount<<",price:"<<price.ToString()<<",source:";
    for(SourceMap::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        ss<<"["<<it->first<<"-"<<it->second<<"]";
    }
    return ss.str();
}


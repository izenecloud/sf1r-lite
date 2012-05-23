#include "product_db.h"
#include <sstream>
using namespace sf1r;

ProductProperty::ProductProperty()
:itemcount(0)
{
}

bool ProductProperty::Parse(const Document& doc)
{
    if(doc.getProperty("itemcount", itemcount))
    {
        if(!doc.getProperty("DOCID", pid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
        if(!doc.getProperty("OID", oid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
    }
    else if(doc.getProperty("uuid", pid)) 
    {
        itemcount = 1;
        if(!doc.getProperty("DOCID", oid))
        {
            LOG(ERROR)<<"parse doc error"<<std::endl;
            return false;
        }
    }
    else
    {
        LOG(ERROR)<<"parse doc error"<<std::endl;
        return false;
    }
    //LOG(INFO)<<"find oid "<<oid<<std::endl;
    UString usource;
    UString uprice;
    doc.getProperty("Source", usource);
    doc.getProperty("Price", uprice);
    price.Parse(uprice);
    if(usource.length()>0)
    {
        std::string ssource;
        usource.convertString(ssource, UString::UTF_8);
        std::vector<std::string> s_vector;
        boost::algorithm::split( s_vector, ssource, boost::algorithm::is_any_of(",") );
        for(uint32_t i=0;i<s_vector.size();i++)
        {
            if(s_vector[i].empty()) continue;
            source.insert(s_vector[i]);
        }
    }
    return true;
}

void ProductProperty::Set(Document& doc) const
{
    doc.property("DOCID") = pid;
    //LOG(INFO)<<"set oid "<<oid<<std::endl;
    doc.property("OID") = oid;
    doc.property("Price") = price.ToUString();
    doc.property("Source") = GetSourceUString();
    doc.property("itemcount") = itemcount;
    doc.eraseProperty("uuid");
}

std::string ProductProperty::GetSourceString() const
{
    std::string result;
    for(SourceType::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        if(!result.empty()) result+=",";
        result += *it;
    }
    return result;
}

izenelib::util::UString ProductProperty::GetSourceUString() const
{
    return izenelib::util::UString(GetSourceString(), izenelib::util::UString::UTF_8);
}

ProductProperty& ProductProperty::operator+=(const ProductProperty& other)
{
    if(pid.empty()) pid = other.pid;
    if(oid.empty() || other.pid==other.oid) oid = other.oid;
    price += other.price;
    for(SourceType::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    {
        source.insert(*oit);
        //SourceMap::iterator it = source.find(oit->first);
        //if(it==source.end())
        //{
            //source.insert(std::make_pair(oit->first, oit->second));
        //}
        //else
        //{
            //it->second += oit->second;
        //}
    }
    itemcount+=other.itemcount;
    return *this;
}

//ProductProperty& ProductProperty::operator-=(const ProductProperty& other)
//{
    //for(SourceMap::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    //{
        //SourceMap::iterator it = source.find(oit->first);
        //if(it==source.end())
        //{
            //source.insert(std::make_pair(oit->first, -1*oit->second));
            ////invalid data
            ////LOG(WARNING)<<"invalid data"<<std::endl;
        //}
        //else
        //{
            //it->second -= oit->second;
            //LOG(INFO)<<oit->first<<" reduced from "<<it->second+oit->second<<" to "<<it->second<<std::endl;
            ////if(it->second<0)
            ////{
                //////invalid data
                ////LOG(WARNING)<<"invalid data"<<std::endl;
                ////it->second = 0;
            ////}
        //}
    //}
    //itemcount -= 1;

    //return *this;
//}


std::string ProductProperty::ToString() const
{
    std::string spid;
    pid.convertString(spid, izenelib::util::UString::UTF_8);
    std::stringstream ss;
    ss<<"pid:"<<spid<<",itemcount:"<<itemcount<<",price:"<<price.ToString()<<",source:";
    for(SourceType::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        ss<<"["<<*it<<"]";
    }
    return ss.str();
}


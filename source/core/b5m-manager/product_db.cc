#include "product_db.h"
#include "product_matcher.h"
#include <document-manager/JsonDocument.h>
#include <mining-manager/util/split_ustr.h>
#include <sstream>
using namespace sf1r;

B5mpDocGenerator::B5mpDocGenerator()
{
    sub_doc_props_.insert("DOCID");
    sub_doc_props_.insert("Title");
    sub_doc_props_.insert("Url");
    sub_doc_props_.insert("Price");
    sub_doc_props_.insert("Picture");
    sub_doc_props_.insert("OriginalPicture");
    sub_doc_props_.insert("Source");
    sub_doc_props_.insert("OriginalCategory");
    sub_doc_props_.insert("PicPrpt");
    sub_doc_props_.insert("SalesAmount");
    sub_doc_props_.insert("isFreeDelivery");
    sub_doc_props_.insert("isCOD");
    sub_doc_props_.insert("isGenuine");
    //sub_doc_props_.clear();
}
void B5mpDocGenerator::Gen(const std::vector<ScdDocument>& odocs, ScdDocument& pdoc)
{
    pdoc.type=UPDATE_SCD;
    int64_t itemcount=0;
    std::set<std::string> psource;
    std::vector<Document> subdocs;
    ProductPrice pprice;
    UString pdate;
    UString spu_title;
    UString url;
    ProductPrice min_price;
    //std::vector<ProductMatcher::Attribute> pattributes;
    bool independent=true;
    UString pid;
    odocs.front().getProperty("uuid", pid);
    //std::cerr<<"b5mp gen "<<pid<<","<<odocs.size()<<std::endl;
    for(uint32_t i=0;i<odocs.size();i++)
    {
        const ScdDocument& doc=odocs[i];
        if(doc.type==NOT_SCD||doc.type==DELETE_SCD)
        {
            continue;
        }
        if(!sub_doc_props_.empty())
        {
            Document subdoc;
            for(ScdDocument::property_const_iterator it=doc.propertyBegin();it!=doc.propertyEnd();++it)
            {
                if(sub_doc_props_.find(it->first)!=sub_doc_props_.end())
                {
                    subdoc.property(it->first) = it->second;
                }
            }
            if(subdoc.getPropertySize()>0)
            {
                subdocs.push_back(subdoc);
            }
        }
        UString oid;
        doc.getProperty("DOCID", oid);
        if(oid!=pid) independent=false;
        doc.getProperty(B5MHelper::GetSPTPropertyName(), spu_title);
        pdoc.merge(doc);
        itemcount+=1;
        UString usource;
        UString uprice;
        //UString uattribute;
        UString udate;
        UString uurl;
        doc.getProperty("Source", usource);
        doc.getProperty("Price", uprice);
        //doc.getProperty("Attribute", uattribute);
        doc.getProperty("DATE", udate);
        doc.getProperty("Url", uurl);
        if(udate.compare(pdate)>0) pdate=udate;
        ProductPrice price;
        price.Parse(uprice);
        pprice+=price;
        if(!min_price.Valid()) 
        {
            url = uurl;
            min_price = price;
        }
        else if(price.Valid())
        {
            if(price.value.first<min_price.value.first)
            {
                min_price=price;
                url = uurl;
            }
        }
        if(usource.length()>0)
        {
            std::string ssource;
            usource.convertString(ssource, UString::UTF_8);
            psource.insert(ssource);
        }
        //if(!uattribute.empty())
        //{
            //std::vector<ProductMatcher::Attribute> attributes;
            //ProductMatcher::ParseAttributes(uattribute, attributes);
            //ProductMatcher::MergeAttributes(pattributes, attributes);
        //}
    }
    if(!spu_title.empty())
    {
        independent=false;
        pdoc.property("Title") = spu_title;
    }
    pdoc.eraseProperty(B5MHelper::GetSPTPropertyName());
    pdoc.eraseProperty("uuid");
    if(!subdocs.empty()&&!independent)
    {
        UString text;
        JsonDocument::ToJsonText(subdocs, text);
        pdoc.property(B5MHelper::GetSubDocsPropertyName()) = text;
    }

    pdoc.property("DOCID") = pid;
    pdoc.property("itemcount") = itemcount;
    pdoc.property("independent") = (int64_t)independent;
    if(!pdate.empty()) pdoc.property("DATE") = pdate;
    pdoc.property("Price") = pprice.ToUString();
    pdoc.property("Url") = url;
    std::string ssource;
    for(std::set<std::string>::const_iterator it=psource.begin();it!=psource.end();++it)
    {
        if(!ssource.empty()) ssource+=",";
        ssource+=*it;
    }
    if(!ssource.empty()) pdoc.property("Source") = UString(ssource, UString::UTF_8);
    //if(!pattributes.empty()) pdoc.property("Attribute") = ProductMatcher::AttributesText(pattributes); 
    if(itemcount==0)
    {
        pdoc.type=DELETE_SCD;
        pdoc.clearExceptDOCID();
    }
}

ProductProperty::ProductProperty()
:itemcount(0), independent(false)
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
    doc.getString("DATE", date);
    SetIndependent();
    //LOG(INFO)<<"find oid "<<oid<<std::endl;
    UString usource;
    UString uprice;
    UString uattribute;
    doc.getProperty("Source", usource);
    doc.getProperty("Price", uprice);
    doc.getProperty("Attribute", uattribute);
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
    std::vector<AttrPair> attrib_list;
    split_attr_pair(uattribute, attrib_list);
    for(std::size_t i=0;i<attrib_list.size();i++)
    {
        std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
        std::vector<std::string> attrib_value_str_list;
        //do duplicate remove
        boost::unordered_set<std::string> value_set;
        for(uint32_t j=0;j<attrib_value_list.size();j++)
        {
            std::string svalue;
            attrib_value_list[j].convertString(svalue, UString::UTF_8);
            if(value_set.find(svalue)!=value_set.end()) continue;
            attrib_value_str_list.push_back(svalue);
            value_set.insert(svalue);
        }
        izenelib::util::UString attrib_name = attrib_list[i].first;
        std::string sname;
        attrib_name.convertString(sname, UString::UTF_8);
        attribute[sname] = attrib_value_str_list;
    }
 
    return true;
}

void ProductProperty::Set(Document& doc) const
{
    doc.property("DOCID") = pid;
    //LOG(INFO)<<"set oid "<<oid<<std::endl;
    doc.property("OID") = oid;
    if(!date.empty())
    {
        doc.property("DATE") = UString(date, UString::UTF_8);
    }
    if(price.Valid())
    {
        doc.property("Price") = price.ToUString();
    }
    izenelib::util::UString usource = GetSourceUString();
    if(!usource.empty())
    {
        doc.property("Source") = usource;
    }

    UString uattribute = GetAttributeUString();
    if(!uattribute.empty())
    {
        doc.property("Attribute") = uattribute;
    }
 
    doc.property("itemcount") = itemcount;
    if(independent)
    {
        doc.property("independent") = (int64_t)1;
    }
    else
    {
        doc.property("independent") = (int64_t)0;
    }
    doc.eraseProperty("uuid");
}

void ProductProperty::SetIndependent()
{
    if(itemcount==1 && pid==oid)
    {
        independent = true;
    }
    else
    {
        independent = false;
    }
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

izenelib::util::UString ProductProperty::GetAttributeUString() const
{
    std::string result;
    for(AttributeType::const_iterator oit = attribute.begin(); oit!=attribute.end(); ++oit)
    {
        if(!result.empty())
        {
            result += ",";
        }
        result += oit->first+":";
        std::string svalue;
        const std::vector<std::string>& value_list = oit->second;
        for(uint32_t i=0;i<value_list.size();i++)
        {
            if(!svalue.empty())
            {
                svalue+="|";
            }
            svalue+=value_list[i];
        }
        result+=svalue;
    }
    return izenelib::util::UString(result, izenelib::util::UString::UTF_8);
}

ProductProperty& ProductProperty::operator+=(const ProductProperty& other)
{
    if(pid.empty()) pid = other.pid;
    if(oid.empty() || other.pid==other.oid) oid = other.oid;
    if(other.date>date) date = other.date; //use latest date
    price += other.price;
    for(SourceType::const_iterator oit = other.source.begin(); oit!=other.source.end(); ++oit)
    {
        source.insert(*oit);
    }
    for(AttributeType::const_iterator oit = other.attribute.begin(); oit!=other.attribute.end(); ++oit)
    {
        attribute[oit->first] = oit->second;
    }
    itemcount+=other.itemcount;
    SetIndependent();
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
    ss<<"pid:"<<spid<<",DATE:"<<date<<",itemcount:"<<itemcount<<",price:"<<price.ToString()<<",source:";
    for(SourceType::const_iterator it = source.begin(); it!=source.end(); ++it)
    {
        ss<<"["<<*it<<"]";
    }
    return ss.str();
}


#include "ScdWriter.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
using namespace sf1r;

ScdWriter::ScdWriter(const std::string& dir, SCD_TYPE scd_type)
:dir_(dir), scd_type_(scd_type)
{
    filename_ = GenSCDFileName(scd_type);
}

ScdWriter::~ScdWriter()
{
}

void ScdWriter::Open_()
{
    ofs_.open( (dir_+"/"+filename_).c_str() );
    output_visitor_.SetOutStream(&ofs_);
}

std::string ScdWriter::GenSCDFileName(SCD_TYPE scd_type)
{
    const static int wait_ms = 100;
    boost::this_thread::sleep( boost::posix_time::milliseconds(wait_ms) );
    //B-00-201109081700-11111-I-C.SCD
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    //20020131T100001,123456789
    std::string ios_str = boost::posix_time::to_iso_string(now);
    std::stringstream ss;
    ss<<"B-00-";
    ss<<ios_str.substr(0,8);
    ss<<ios_str.substr(9,4);
    ss<<"-";
    ss<<ios_str.substr(13,2);
    ss<<ios_str.substr(16,3);
    ss<<"-";
    ss<<ScdParser::SCD_TYPE_FLAGS[scd_type];
    ss<<"-C.SCD";
    return ss.str();
}

bool ScdWriter::Append(const Document& doc)
{
    const static std::string DOCID = "DOCID";
    Document::property_const_iterator docid_it = doc.findProperty(DOCID);
    if(docid_it == doc.propertyEnd())
    {
        return false;
    }
    if(!ofs_.is_open())
    {
        Open_();
    }
    std::string sdocid = propstr_to_str(docid_it->second.getPropertyStrValue());
    ofs_<<"<"<<DOCID<<">"<<sdocid<<std::endl;
    Document::property_const_iterator it = doc.propertyBegin();
    while(it!=doc.propertyEnd())
    {
        if(it->first==DOCID)
        {
            ++it;
            continue;
        }
//         if(pname_filter_)
//         {
//             if(!pname_filter_(it->first))
//             {
//                 ++it;
//                 continue;
//             }
//         }
        ofs_<<"<"<<it->first<<">";
        boost::apply_visitor( output_visitor_, it->second.getVariant());
        ofs_<<std::endl;
        ++it;
    }
    return true;
}

bool ScdWriter::Append(const SCDDoc& doc)
{
    if(scd_type_!=DELETE_SCD)
    {
        if(doc.size()<2)//at least two properties including DOCID in I and U scd
        {
            std::cout<<"at least two properties including DOCID in I and U document"<<std::endl;
            return false;
        }
    }
    else
    {
        if(doc.size()!=1)
        {
            std::cout<<"D document only need the DOCID property"<<std::endl;
            return false;
        }
    }
    bool docid_found = false;
    std::string docid_value;
    for(SCDDoc::const_iterator it = doc.begin();it!=doc.end();++it)
    {
        const std::string& name = it->first;
        if(name=="DOCID")
        {
            docid_value = propstr_to_str(it->second);
            docid_found = true;
            break;
        }
    }
    if(!docid_found)
    {
        std::cout<<"DOCID property not found"<<std::endl;
        return false;
    }
    if(!ofs_.is_open())
    {
        Open_();
    }
    ofs_<<"<DOCID>"<<docid_value<<std::endl;
    if(scd_type_!=DELETE_SCD)
    {
        for(SCDDoc::const_iterator it = doc.begin();it!=doc.end();++it)
        {
            const std::string& name = it->first;
            if(name!="DOCID")
            {
                std::string value = propstr_to_str(it->second);
                ofs_<<"<"<<name<<">"<<value<<std::endl;
            }
        }
    }
    return true;
}

void ScdWriter::Close()
{
    if(ofs_.is_open())
    {
        ofs_.close();
    }
}

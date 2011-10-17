#include "ScdWriter.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace sf1r;

ScdWriter::ScdWriter(const std::string& dir, int op)
{
    std::string filename = GenSCDFileName(op);
    ofs_.open( (dir+"/"+filename).c_str() );
}

ScdWriter::~ScdWriter()
{
}

std::string ScdWriter::GenSCDFileName( int op)
{
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
    if(op==UPDATE_SCD)
    {
        ss<<"U";
    }
    else if(op==DELETE_SCD)
    {
        ss<<"D";
    }
    else
    {
        ss<<"I";
    }
    ss<<"-C.SCD";
    return ss.str();
}
    
void ScdWriter::Append(const Document& doc)
{
    Document::property_const_iterator it = doc.propertyBegin();
    while(it!=doc.propertyEnd())
    {
//         if(pname_filter_)
//         {
//             if(!pname_filter_(it->first))
//             {
//                 ++it;
//                 continue;
//             }
//         }
        bool is_ustr = boost::apply_visitor( ustring_visitor_, it->second.getVariant());
        if(!is_ustr) 
        {
            ++it;
            continue;
        }
        std::string value;
        it->second.get<izenelib::util::UString>().convertString(value, izenelib::util::UString::UTF_8);
        ofs_<<"<"<<it->first<<">"<<value<<std::endl<<std::endl;
        ++it;
    }
}
    
void ScdWriter::Close()
{
    ofs_.close();
}

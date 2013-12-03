#ifndef SF1R_TEST_PMTESTPARSER_H
#define SF1R_TEST_PMTESTPARSER_H

#include <common/type_defs.h>


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include <product-manager/pm_def.h>
#include <product-manager/pm_types.h>
#include <product-manager/pm_config.h>
#include <product-manager/product_price.h>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

using namespace sf1r;

struct PMTestResultItem
{
    int op;
    ProductPrice price;
    uint32_t itemcount;
    uint32_t uuid_docid;
};


class PMTestParser
{
public:
    static void Split(const std::string& input, char spliter, std::vector<std::string>& result)
    {
        if(input.length()==0) return;
        std::size_t pos = 0;
        while(true)
        {
            std::size_t f = input.find(spliter, pos);
            if(f==std::string::npos)
            {
                std::string str = input.substr(pos, f);
                result.push_back(str);
                break;
            }
            else
            {
                std::string str = input.substr(pos, f-pos);
                result.push_back(str);
                pos = f+1;
            }
        }
//         std::cout<<"split input : "<<input<<std::endl;
//         for(uint32_t i=0;i<result.size();i++)
//         {
//             std::cout<<result[i]<<std::endl;
//         }
    }
    
    static void ParseResult(const std::string& line, PMTestResultItem& item)
    {
        std::vector<std::string> vec;
        Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==4, "Not valid line "+line);
        item.op = boost::lexical_cast<int>(vec[0]);
        item.price = ProductPrice(-1.0, -1.0);
        if(vec[1].length()>0)
        {
            izenelib::util::UString text(vec[1], izenelib::util::UString::UTF_8);
//             std::cout<<"before parse price"<<std::endl;
            item.price.Parse(text);
//             std::cout<<"after parse price"<<std::endl;
        }
        item.itemcount = 0;
        if(vec[2].length()>0)
        {
            item.itemcount = boost::lexical_cast<uint32_t>(vec[2]);
        }
        item.uuid_docid = 0;
        if(vec[3].length()>0)
        {
            item.uuid_docid = boost::lexical_cast<uint32_t>(vec[3]);
        }
        
    }
};

class PMTestParserI
{
public:
    static void Parse(const std::string& line, PMDocumentType& doc)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==3, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="I" );
        uint32_t docid = boost::lexical_cast<uint32_t>(vec[1]);
        doc.setId(docid);
        std::string sdocid = "docid"+boost::lexical_cast<std::string>(docid);
        doc.property("DOCID") = str_to_propstr(sdocid);
        doc.property("Price") = str_to_propstr(vec[2]);
    }
};

class PMTestParserU
{
public:
    static void Parse(const std::string& line, PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, bool& r_type)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==5, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="U" );
        uint32_t docid = boost::lexical_cast<uint32_t>(vec[1]);
        doc.setId(docid);
        std::string sdocid = "docid"+boost::lexical_cast<std::string>(docid);
        doc.property("DOCID") = str_to_propstr(sdocid);
        doc.property("Price") = str_to_propstr(vec[2]);
        uint32_t oldid = boost::lexical_cast<uint32_t>(vec[3]);
        index_document.setOldId(oldid);
        int ir = boost::lexical_cast<int>(vec[4]);
        r_type = ir>0?true:false;
    }
};

class PMTestParserD
{
public:
    static void Parse(const std::string& line, uint32_t& docid)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==2, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="D" );
        docid = boost::lexical_cast<uint32_t>(vec[1]);
    }
};

class PMTestParserC
{
public:
    static void Parse(const std::string& line, std::vector<uint32_t>& docid_list, bool& succ)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==3, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="C" );
        std::vector<std::string> sdoclist;
        PMTestParser::Split(vec[1], ':', sdoclist);
        docid_list.resize(sdoclist.size());
        for(uint32_t i=0;i<sdoclist.size();i++)
        {
            docid_list[i] = boost::lexical_cast<uint32_t>(sdoclist[i]);
        }
        int ir = boost::lexical_cast<int>(vec[2]);
        succ = ir>0?true:false;
//         std::cout<<"PMTestParserC"<<std::endl;
//         for(uint32_t i=0;i<docid_list.size();i++)
//         {
//             std::cout<<docid_list[i]<<std::endl;
//         }
//         std::cout<<(int)succ<<std::endl;
    }
};

class PMTestParserA
{
public:
    static void Parse(const std::string& line, uint32_t& uuid_docid, std::vector<uint32_t>& docid_list, bool& succ)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==4, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="A" );
        uuid_docid = boost::lexical_cast<uint32_t>(vec[1]);
        std::vector<std::string> sdoclist;
        PMTestParser::Split(vec[2], ':', sdoclist);
        docid_list.resize(sdoclist.size());
        for(uint32_t i=0;i<sdoclist.size();i++)
        {
            docid_list[i] = boost::lexical_cast<uint32_t>(sdoclist[i]);
        }
        int ir = boost::lexical_cast<int>(vec[3]);
        succ = ir>0?true:false;
    }
};

class PMTestParserR
{
public:
    static void Parse(const std::string& line, uint32_t& uuid_docid, std::vector<uint32_t>& docid_list, bool& succ)
    {
        std::vector<std::string> vec;
        PMTestParser::Split(line, ',', vec);
        BOOST_CHECK_MESSAGE( vec.size()==4, "Not valid line "+line);
        BOOST_CHECK( vec[0]=="R" );
        
        uuid_docid = boost::lexical_cast<uint32_t>(vec[1]);
        std::vector<std::string> sdoclist;
        PMTestParser::Split(vec[2], ':', sdoclist);
        docid_list.resize(sdoclist.size());
        for(uint32_t i=0;i<sdoclist.size();i++)
        {
            docid_list[i] = boost::lexical_cast<uint32_t>(sdoclist[i]);
        }
        int ir = boost::lexical_cast<int>(vec[3]);
        succ = ir>0?true:false;
    }
};


#endif

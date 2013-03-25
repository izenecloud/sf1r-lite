#include <boost/test/unit_test.hpp>
#include <common/ScdParser.h>
#include "ScdBuilder.h"

#include <iostream>
#include <string>
#include <common/ScdTypeWriter.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <util/ustring/UString.h>
#include <b5m-manager/b5m_helper.h>
using namespace sf1r;
using namespace std;
using namespace boost;
using namespace izenelib::util;
BOOST_AUTO_TEST_SUITE(SCDTypeWriter_test)


void checkDoc(Document doc1,Document doc2)
{
    BOOST_CHECK_EQUAL(    doc1.property("Title") ,    doc2.property("Title") );
    BOOST_CHECK_EQUAL(    doc1.property("DOCID") ,    doc2.property("DOCID") );
    BOOST_CHECK_EQUAL(    doc1.property("Content") ,    doc2.property("Content") );
}
BOOST_AUTO_TEST_CASE(SCDTypeWirter_Append)
{
    boost::filesystem::remove_all("./scd_type_test");
    boost::filesystem::create_directories("./scd_type_test");

    ScdTypeWriter  scd("./scd_type_test");
    vector<vector<Document> >  AllDoc;
    AllDoc.resize(4);

    Document doc;
    SCD_TYPE type=INSERT_SCD;
    for(unsigned i=0; i<10000; i++)
    {
        doc.property("Title") = UString(boost::lexical_cast<string>(rand()), UString::UTF_8);
        doc.property("DOCID") = UString(boost::lexical_cast<string>(rand()),  UString::UTF_8);
        doc.property("Content") =UString(boost::lexical_cast<string>(rand()), UString::UTF_8);
        int itype=rand()%4;
        if(itype==0)
        {
            type=INSERT_SCD;
        }
        else if(itype==1)
        {
            type=DELETE_SCD;
        }
        else if(itype==2)
        {
            type=UPDATE_SCD;
        }
        else
        {
            type=NOT_SCD;
        }
        AllDoc[itype].push_back(doc);
        scd.Append(doc,type);

    }

    std::vector<std::string> scd_list;
    B5MHelper::GetScdList("./scd_type_test", scd_list);
    for(uint32_t i=0; i<scd_list.size(); i++)
    {
        std::string scd_file = scd_list[i];
        SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        vector<Document>  Doc;
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
                doc_iter!= parser.end(); ++doc_iter, ++n)
        {

            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Doc.push_back(doc);


        }
        int itype=0;
        if(scd_type==INSERT_SCD)
        {
            itype=0;
        }
        else if(scd_type==DELETE_SCD)
        {
            itype=1;
        }
        else if(scd_type==UPDATE_SCD)
        {
            itype=2;
        }
        else
        {
            BOOST_CHECK_EQUAL(false,true);
        }
        BOOST_CHECK_EQUAL( Doc.size() , AllDoc[itype].size());
        for(unsigned j=0; j<Doc.size(); j++)
        {
            checkDoc(Doc[j],AllDoc[itype][j]);
        }
    }

}

BOOST_AUTO_TEST_SUITE_END()

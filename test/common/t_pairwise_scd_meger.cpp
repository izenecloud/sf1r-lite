#include <boost/test/unit_test.hpp>
#include <common/ScdParser.h>
#include "ScdBuilder.h"
#include <b5m-manager/b5m_helper.h>
#include <iostream>
#include <string>
#include <common/ScdTypeWriter.h>
#include <common/PairwiseScdMerger.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <util/ustring/UString.h>

#include <b5m-manager/b5mp_processor.h>
using namespace sf1r;
using namespace std;
using namespace boost;
using namespace izenelib::util;
std::map<uint128_t,PairwiseScdMerger::ValueType > idValuemap,resultMap;
void Output_(PairwiseScdMerger::ValueType& value, int status)
{
}

void OutputAll_()
{
}
void FilterTask_(PairwiseScdMerger::ValueType& value)
{
}
uint128_t GetDocId_(const Document& doc, const std::string& pname="DOCID")
{
    std::string sdocid;
    doc.getString(pname, sdocid);
    if(sdocid.empty()) return 0;
    return B5MHelper::StringToUint128(sdocid);
}

void scdinit(ScdTypeWriter& scd,int ty=0)
{
    vector< vector<Document> > vecdoc;
    vecdoc.resize(3);
    Document doc;
    SCD_TYPE type=INSERT_SCD;
    for(unsigned i=0; i<10000; i++)
    {
        doc.property("Title") = UString(boost::lexical_cast<string>(rand()), UString::UTF_8);
        doc.property("DOCID") = UString(boost::lexical_cast<string>(rand()%1000000),  UString::UTF_8);
        doc.property("uuid") = UString(boost::lexical_cast<string>(rand()%10),  UString::UTF_8);
        doc.property("Content") =UString(boost::lexical_cast<string>(rand()), UString::UTF_8);
        int itype=rand()%3;
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
        scd.Append(doc,type);
        if(type==INSERT_SCD)
        {
            vecdoc[0].push_back(doc);
        }
        else if(type==UPDATE_SCD)
        {
            vecdoc[1].push_back(doc);
        }
        else if(type==DELETE_SCD)
        {
            vecdoc[2].push_back(doc);
        }



    };
    if(ty>0)
    {
        for(unsigned i=0; i<vecdoc[0].size(); i++)
        {
            PairwiseScdMerger::ValueType value(vecdoc[0][i], INSERT_SCD);
            uint128_t id = GetDocId_(vecdoc[0][i]);
            idValuemap.insert(std::make_pair(id,value));
            idValuemap[id]=value;
        }
        for(unsigned i=0; i<vecdoc[2].size(); i++)
        {
            PairwiseScdMerger::ValueType value(vecdoc[2][i], DELETE_SCD);
            uint128_t id = GetDocId_(vecdoc[2][i]);
            idValuemap.insert(std::make_pair(id,value));
            idValuemap[id]=value;
        }
        for(unsigned i=0; i<vecdoc[1].size(); i++)
        {
            PairwiseScdMerger::ValueType value(vecdoc[1][i], UPDATE_SCD);
            uint128_t id = GetDocId_(vecdoc[1][i]);
            idValuemap.insert(std::make_pair(id,value));
            idValuemap[id]=value;
        }

    }

}

vector< vector<Document> > getDoc(string scd_path_)
{

    vector< vector<Document> > vecdoc;
    vecdoc.resize(3);
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path_, scd_list);
    ScdParser parser(izenelib::util::UString::UTF_8);
    for(uint32_t s=0; s<scd_list.size(); s++)
    {
        std::string scd_file = scd_list[s];
        ScdParser parser(izenelib::util::UString::UTF_8);
        SCD_TYPE type = ScdParser::checkSCDType(scd_file);
        parser.load(scd_file);
        for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
        {

            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }

            PairwiseScdMerger::ValueType value(doc, type);
            uint128_t id = GetDocId_(doc);
            resultMap.insert(std::make_pair(id,value));
            if(type==INSERT_SCD)
            {
                vecdoc[0].push_back(doc);
            }
            else if(type==UPDATE_SCD)
            {
                vecdoc[1].push_back(doc);
            }
            else if(type==DELETE_SCD)
            {
                vecdoc[2].push_back(doc);
            }
            else
            {
                cout<<"error type"<<type<<endl;
            }

        }
    }
    return vecdoc;
}

void checkDoc(Document doc1,Document doc2)
{
    BOOST_CHECK_EQUAL(    doc1.property("Title") ,    doc2.property("Title") );
    BOOST_CHECK_EQUAL(    doc1.property("DOCID") ,    doc2.property("DOCID") );
    BOOST_CHECK_EQUAL(    doc1.property("Content") ,    doc2.property("Content") );
}
BOOST_AUTO_TEST_SUITE(pairwise_test)
BOOST_AUTO_TEST_CASE(pairwise_Append)
{
    boost::filesystem::remove_all("./pairewise_test");
    boost::filesystem::create_directories("./pairewise_test");
    boost::filesystem::remove_all("./scd_exist");
    boost::filesystem::create_directories("./scd_exist");
    uint32_t m=4;
    ScdTypeWriter  scd("./pairewise_test");
    ScdTypeWriter  scdexist("./scd_exist");
    scdinit(scd,1);
    scdinit(scdexist);
    PairwiseScdMerger merger("./pairewise_test");
    merger.SetM(m);
    merger.SetMProperty("uuid");
    merger.SetExistScdPath("./scd_exist");

    std::string output_dir = B5MHelper::GetB5moMirrorPath("./pairewise_test");
    B5MHelper::PrepareEmptyDir(output_dir);
    merger.SetOutputPath(output_dir);
    merger.SetOutputer(boost::bind( &Output_,  _1, _2));
    merger.SetMEnd(boost::bind( &OutputAll_));
    merger.SetPreprocessor(boost::bind( &FilterTask_, _1));
    merger.Run();
    getDoc("./pairewise_test");
    BOOST_CHECK_EQUAL(    resultMap.size(),   idValuemap.size() );
    for ( std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator idv = idValuemap.begin(); idv != idValuemap.end(); idv++ )
    {
        std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator i=resultMap.find(idv->first);
        BOOST_CHECK_EQUAL(    i==resultMap.end(),   false );
        BOOST_CHECK_EQUAL(    (i->second).type,(idv->second).type );
    }
    boost::filesystem::remove_all("./pairewise_test");
    boost::filesystem::remove_all("./scd_exist");
}




BOOST_AUTO_TEST_SUITE_END()

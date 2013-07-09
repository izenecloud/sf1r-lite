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
std::map<uint128_t,PairwiseScdMerger::ValueType > idValuemap,resultMap,IteridValuemap;
std::vector<int> existdocidSet;
std::map<uint128_t,vector<int> > statusMap,statusMapForCheck;
enum {OLD = 1, EXIST, REPLACE, NEW};

uint128_t GetDocId_(const Document& doc, const std::string& pname="DOCID")
{
    std::string sdocid;
    doc.getString(pname, sdocid);
    if(sdocid.empty()) return 0;
    return B5MHelper::StringToUint128(sdocid);
}
string GetDocId(const Document& doc, const std::string& pname="DOCID")
{
    std::string sdocid;
    doc.getString(pname, sdocid);
    if(sdocid.empty()) return "0";
    return sdocid;
}
void Output_(PairwiseScdMerger::ValueType& value, int status)
{

    uint128_t id=GetDocId_(value.doc);
    if(statusMap.find(id) == statusMap.end())
    {
        vector<int> a;
        statusMap[id]=a;
    }

    statusMap[id].push_back(status);

}

void OutputAll_()
{
}
void FilterTask_(PairwiseScdMerger::ValueType& value)
{
}
void scdinit(ScdTypeWriter& scd,unsigned docnum,int ty=0 )
{

    vector< vector<Document> > vecdoc;
    vector< vector<int> > idvec;
    vecdoc.resize(3);
    idvec.resize(3);
    Document doc;
    SCD_TYPE type=INSERT_SCD;
    std::vector<int> docidSet;
    for(unsigned i=0; i<docnum; i++)
    {
        doc.property("Title") = str_to_propstr(boost::lexical_cast<string>(rand()), UString::UTF_8);
        int docid=rand()%100000;
        int itype=rand()%3;
        if(docidSet.size()<docnum*1/2)
        {
            itype=0;
        }
        else if(docidSet.size()<docnum*3/4)
        {
            itype=1;
        }
        else
        {
            itype=2;
        }//-ty;}
        if(itype==0)
        {

            if(ty==1)
            {
                while(find(docidSet.begin(),docidSet.end(),docid)!=docidSet.end())
                {
                    docid=rand()%100000;
                }
            }
            else
            {
                while(find(existdocidSet.begin(),existdocidSet.end(),docid)!=existdocidSet.end())
                {
                    docid=rand()%100000;
                }
            }
        }
        else
        {
            while(find(idvec[itype].begin(),idvec[itype].end(),docid)!=idvec[itype].end())
            {
               docid=rand()%100000;
            }
            /*
            docid=existdocidSet[rand()%existdocidSet.size()];
            while(find(idvec[itype].begin(),idvec[itype].end(),docid)!=idvec[itype].end())
            {
                docid=existdocidSet[rand()%existdocidSet.size()];
            }
            */
        }
        docidSet.push_back(docid);
        existdocidSet.push_back(docid);
        doc.property("DOCID") = str_to_propstr(boost::lexical_cast<string>(docid),  UString::UTF_8);
        doc.property("uuid") = str_to_propstr(boost::lexical_cast<string>(docid%3000),  UString::UTF_8);
        doc.property("Content") = str_to_propstr(boost::lexical_cast<string>(rand()), UString::UTF_8);

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
            idvec[0].push_back(docid);
        }
        else if(type==UPDATE_SCD)
        {
            vecdoc[1].push_back(doc);
            idvec[2].push_back(docid);
        }
        else if(type==DELETE_SCD)
        {
            vecdoc[2].push_back(doc);
            idvec[1].push_back(docid);
        }



    };


    for(unsigned i=0; i<vecdoc[0].size(); i++)
    {
        PairwiseScdMerger::ValueType value(vecdoc[0][i], INSERT_SCD);
        uint128_t id = GetDocId_(vecdoc[0][i]);
        std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator it=idValuemap.find(id);
        idValuemap.insert(std::make_pair(id,value));
        idValuemap[id]=value;
        IteridValuemap.insert(std::make_pair(id,value));
        IteridValuemap[id]=value;
        if(ty==0)
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;
                statusMapForCheck[id].push_back(EXIST);
            }
            else
            {
                statusMapForCheck[id].push_back(EXIST);
            }

        }
        else
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;
                statusMapForCheck[id].push_back(REPLACE);

            }
            else
            {
                for(unsigned i=0; i<statusMapForCheck[id].size(); i++)
                {

                    if(statusMapForCheck[id][i]!=EXIST)
                    {
                        break;
                    }
                    statusMapForCheck[id][i]=OLD;

                }
                if(statusMapForCheck[id][statusMapForCheck[id].size()-1]!=REPLACE)
                    statusMapForCheck[id].push_back(REPLACE);
            }
        }

    }

    for(unsigned i=0; i<vecdoc[2].size(); i++)
    {
        PairwiseScdMerger::ValueType value(vecdoc[2][i], DELETE_SCD);
        uint128_t id = GetDocId_(vecdoc[2][i]);
        std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator it=idValuemap.find(id);
        idValuemap.insert(std::make_pair(id,value));
        idValuemap[id]=value;
        IteridValuemap.insert(std::make_pair(id,value));
        IteridValuemap[id]=value;
        if(ty==0)
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;

            }

            statusMapForCheck[id].push_back(EXIST);
        }
        else
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;
                statusMapForCheck[id].push_back(REPLACE);

            }
            else
            {
                for(unsigned i=0; i<statusMapForCheck[id].size(); i++)
                {



                    if(statusMapForCheck[id][i]!=EXIST)
                    {
                        break;
                    }
                    statusMapForCheck[id][i]=OLD;
                }
                if(statusMapForCheck[id][statusMapForCheck[id].size()-1]!=REPLACE)
                    statusMapForCheck[id].push_back(REPLACE);
            }
        }
    }

    for(unsigned i=0; i<vecdoc[1].size(); i++)
    {
        PairwiseScdMerger::ValueType value(vecdoc[1][i], UPDATE_SCD);

        uint128_t id = GetDocId_(vecdoc[1][i]);
        std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator it=idValuemap.find(id);
        if(ty==0)
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;
            }

            statusMapForCheck[id].push_back(EXIST);
        }
        else
        {
            if(statusMapForCheck.find(id) == statusMapForCheck.end())
            {
                vector<int> a;
                statusMapForCheck[id]=a;
                statusMapForCheck[id].push_back(REPLACE);

            }
            else
            {

                for(unsigned i=0; i<statusMapForCheck[id].size(); i++)
                {

                    if(statusMapForCheck[id][i]!=EXIST)
                    {
                        break;
                    }
                    statusMapForCheck[id][i]=OLD;
                }
                if(statusMapForCheck[id][statusMapForCheck[id].size()-1]!=REPLACE)
                    statusMapForCheck[id].push_back(REPLACE);
            }

        }
        if(it!=idValuemap.end()&&ty==1)
        {

            if(it->second.type==INSERT_SCD)
            {

                idValuemap.insert(std::make_pair(id,value));
                value.type=INSERT_SCD;
                idValuemap[id]=value;

            }
            else if(it->second.type==DELETE_SCD)
            {

                idValuemap.insert(std::make_pair(id,value));
                idValuemap[id]=value;

            }
            else if(it->second.type==UPDATE_SCD)
            {

                idValuemap.insert(std::make_pair(id,value));
                value.type=INSERT_SCD;
                idValuemap[id]=value;

            }
            else
            {

                idValuemap.insert(std::make_pair(id,value));
                idValuemap[id]=value;

            }

        }
        else
        {
            if(it!=idValuemap.end())
            {
                if(it->second.type==INSERT_SCD)
                {
                    idValuemap.insert(std::make_pair(id,value));
                    idValuemap[id]=value;
                }
                else if(it->second.type==DELETE_SCD)
                {
                }
                else
                {
                    //idValuemap.insert(std::make_pair(id,value));
                    //idValuemap[id]=value;

                }

            }
            else
            {
                value.type= RTYPE_SCD;
                idValuemap.insert(std::make_pair(id,value));
                idValuemap[id]=value;
            }
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
            if(resultMap[id].type!=DELETE_SCD)
            {
                resultMap[id]=value;
            }
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

    for(unsigned i=0; i<10; i++)
    {
        time_t t;

        srand((unsigned)time(&t));
        idValuemap.clear();
        resultMap.clear();
        existdocidSet.clear();
        IteridValuemap.clear();
        statusMap.clear();
        statusMapForCheck.clear();
        boost::filesystem::remove_all("./pairewise_test");
        boost::filesystem::create_directories("./pairewise_test");
        boost::filesystem::remove_all("./scd_exist");
        boost::filesystem::create_directories("./scd_exist");
        uint32_t m=rand()%10+2;
        ScdTypeWriter  scd("./pairewise_test");


        for(int j=0; j<rand()%20+1; j++)
        {
            ScdTypeWriter  scdexist("./scd_exist");
            scdinit(scdexist,10000);
        }
        scdinit(scd,4000,1);


        PairwiseScdMerger merger("./pairewise_test");
        merger.SetExistScdPath("./scd_exist");
        merger.SetM(m);
        merger.SetMProperty("uuid");


        std::string output_dir = B5MHelper::GetB5moMirrorPath("./pairewise_test");
        B5MHelper::PrepareEmptyDir(output_dir);
        merger.SetOutputPath(output_dir);
        merger.SetOutputer(boost::bind( &Output_,  _1, _2));
        merger.SetMEnd(boost::bind( &OutputAll_));
        merger.SetPreprocessor(boost::bind( &FilterTask_, _1));
        merger.Run();
        vector< vector<Document> > vecdoc=getDoc(B5MHelper::GetB5moMirrorPath("./pairewise_test"));
        // cout<<  resultMap.size()<<" " <<idValuemap.size()<<endl;
        // cout<<  vecdoc[0].size()<<" " <<vecdoc[1].size()<<"  "<<vecdoc[2].size()<<endl;
        BOOST_CHECK_EQUAL(    resultMap.size(),   idValuemap.size() );
        BOOST_CHECK_EQUAL(    statusMap.size(),   statusMapForCheck.size() );
        int a=0,b=0,c=0,d=0,e=0,f=0;
        for ( std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator idv = idValuemap.begin(); idv != idValuemap.end(); idv++ )
        {
            std::map<uint128_t,PairwiseScdMerger::ValueType >::iterator it=resultMap.find(idv->first);
            BOOST_CHECK_EQUAL(    it==resultMap.end(),   false );
            if( (idv->second).type== RTYPE_SCD){(idv->second).type=UPDATE_SCD;}
            BOOST_CHECK_EQUAL(    (it->second).type,(idv->second).type );
            if((it->second).type!=(idv->second).type ){cout<<B5MHelper::Uint128ToString(idv->first)<<endl;i=10;}
            //BOOST_CHECK_EQUAL(    GetDocId(    (i->second).doc),GetDocId( (idv->second).doc) );
            if((it->second).type==1)
            {
                a++;
            }
            if((it->second).type==2)
            {
                f++;
            }
            if((it->second).type==4)
            {
                b++;
            }
            if((idv->second).type==1)
            {
                c++;
            }
            if((idv->second).type==4)
            {
                d++;
            }
            if((idv->second).type==2)
            {
                e++;
            }
        }
        cout<<statusMap.size()<<"  "<<statusMapForCheck.size()<<endl;
        std::ofstream out;

        for ( std::map<uint128_t,vector<int> >::iterator idv = statusMap.begin(); idv != statusMap.end(); idv++ )
        {
            std::map<uint128_t,vector<int> >::iterator i=statusMapForCheck.find(idv->first);
            BOOST_CHECK_EQUAL(    i!=statusMapForCheck.end(),true );
            BOOST_CHECK_EQUAL(    (i->second).size(),(idv->second).size() );

            if( (i->second).size()!=(idv->second).size())
                cout<<B5MHelper::Uint128ToString(idv->first)<<"status";
            for(unsigned j=0; j<(idv->second).size(); j++)
            {
                if( (i->second).size()!=(idv->second).size())
                    cout<<(idv->second)[j]<<" ";
                BOOST_CHECK_EQUAL( (i->second)[j],(idv->second)[j] );
            }
            //cout<<endl;
        }


        string dst="./scd_exist/";
        string src="./pairewise_test/";
        std::vector<std::string> scd_list;
        B5MHelper::GetScdList(src, scd_list);
        /*
            for(unsigned i=0;i<scd_list.size();i++)
            {
                if (boost::filesystem::is_regular_file(scd_list[i]))
                {
                    cout<<(dst+scd_list[i].substr(17))<<endl;
                    boost::filesystem::copy_file(scd_list[i], dst+scd_list[i].substr(17), boost::filesystem::copy_option::overwrite_if_exists);
                }
            }
        */

    }

    /*
    output_dir = B5MHelper::GetB5moMirrorPath("./pairewise_test")+"2";
    merger.SetOutputPath(output_dir);
    B5MHelper::PrepareEmptyDir(output_dir);
    merger.Run();
    */
    //boost::filesystem::remove_all("./pairewise_test");
    //boost::filesystem::remove_all("./scd_exist");
}




BOOST_AUTO_TEST_SUITE_END()

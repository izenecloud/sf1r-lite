#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
#include <b5m-manager/comment_db.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
using namespace sf1r;
using namespace std;
using namespace boost;

BOOST_AUTO_TEST_SUITE(Commentdb_test)


BOOST_AUTO_TEST_CASE(Insert_test)
//int main()
{
    //system("rm ./cdb");
    CommentDb cdb_("./cdb");
    cdb_.open();
    //std::cout<<"running a case"<<std::endl;
    std::cout<<"cdb open"<< cdb_.is_open()<<std::endl;
    BOOST_CHECK_EQUAL(cdb_.is_open(), true);
    BOOST_TEST_MESSAGE("cdb open");
    uint128_t key;
    //uint128_t limit=0-1;
    std::vector<uint128_t> veckey;
    for(unsigned int i=0;i<100000;i++)
    { 
        key=random()*100000000000000;
        cdb_.Insert(key);
        veckey.push_back(key);

        //cdb_.Get(key,pidmirror);
        bool ret=cdb_.Get(key);
        //BOOST_CHECK_EQUAL(ret, false);
        if(ret)
        { 
        //  std::cout<<"find an error1"<<std::endl;
        //  return 0;
        }

    }
    cdb_.flush();
    for(std::vector<uint128_t>::iterator it=veckey.begin();it!=veckey.end();it++)
    {
        key=(*it);
        bool ret=cdb_.Get(key);
        BOOST_CHECK_EQUAL(ret,true);
    }
    int misjudge=0,total=0;
    for(unsigned int i=0;i<100000;i++)
    { 
        key=random()*100000000000000;
        if(find(veckey.begin(),veckey.end(),key)==veckey.end())
        {
            total++;
            bool ret=cdb_.Get(key);
            BOOST_CHECK_EQUAL(ret,false);
            if(ret)
            {
               misjudge++;
            }
        }



    }
    BOOST_CHECK_EQUAL(double(misjudge/total)<0.000001,true);
        //BOOST_CHECK_EQUAL(pid==pidmirror,true);100000
}

BOOST_AUTO_TEST_SUITE_END() 

//BOOST_AUTO_TEST_SUITE_END()


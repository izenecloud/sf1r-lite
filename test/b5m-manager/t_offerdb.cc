#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
#include <b5m-manager/offer_db.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
using namespace sf1r;
using namespace std;
using namespace boost;

BOOST_AUTO_TEST_SUITE(odb_test)


BOOST_AUTO_TEST_CASE(odbInsert_test)
{

    boost::filesystem::remove_all("./odb");
    OfferDb odb_("./odb");
    odb_.open();
    std::cout<<"running a case"<<std::endl;
    std::cout<<"ODB open"<< odb_.is_open()<<std::endl;
    BOOST_CHECK_EQUAL(odb_.is_open(), true);
    BOOST_TEST_MESSAGE("ODB open");
    uint128_t oid;
    uint128_t pid;
    std::vector<std::pair<uint128_t,uint128_t> > o2pmap;
    for(unsigned int i=0;i<100;i++)
    { 
        oid= rand();
        pid= rand();
        odb_.insert(oid,pid);
        o2pmap.push_back(std::make_pair(oid,pid));
        uint128_t pidmirror;
        //odb_.get(oid,pidmirror);
        bool ret=odb_.get(oid,pidmirror);
        //BOOST_CHECK_EQUAL(ret, false);
        
    }
    odb_.flush();
    for(std::vector<std::pair<uint128_t,uint128_t> >::iterator it=o2pmap.begin();it!=o2pmap.end();it++)
    {
        oid=(*it).first;
        pid=(*it).second;
        uint128_t pidmirror;
        bool ret=odb_.get(oid,pidmirror);
        BOOST_CHECK_EQUAL(ret,true);
        BOOST_CHECK_EQUAL(pid==pidmirror,true);
        
    }

}


BOOST_AUTO_TEST_SUITE_END()


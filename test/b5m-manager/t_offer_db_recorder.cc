#include <b5m-manager/offer_db.h>
#include <b5m-manager/offer_db_recorder.h>
#include <boost/test/unit_test.hpp>

using namespace sf1r;
using namespace sf1r::b5m;
using namespace std;

BOOST_AUTO_TEST_SUITE(odb_test)

BOOST_AUTO_TEST_CASE(Insert_test)
{
    boost::filesystem::remove_all("./odb");
    boost::filesystem::remove_all("./lastodb");
    boost::shared_ptr<OfferDb> odb;
    boost::shared_ptr<OfferDb> last_odb;
    

    
        std::string last_odb_path = "./lastodb";
        std::string odb_path = "./odb";
        std::cout << "odb path: " << odb_path <<std::endl;
        odb.reset(new OfferDb(odb_path));
     

    cout<<"build"<<endl;

        std::cout << "last odb path: " << last_odb_path <<std::endl;
        last_odb.reset(new OfferDb(last_odb_path));


    cout<<"last build"<<endl;

    string value;
    uint128_t oid,pid;

    std::vector<boost::tuple<uint128_t,uint128_t,uint128_t> > o2pmap;
    odb->open();
    last_odb->open();
    for(unsigned int i=0;i<10000;i++)
    { 

        oid= int(rand()%1000000000000);
        pid= int(rand()%1000000000000);
        odb->insert(oid,pid);

        //o2pmap.push_back(std::make_pair(oid,pid));


        //odb->get(oid,pidmirror);
        uint128_t increasepid= floor(rand()%2);
        last_odb->insert(oid,pid+increasepid);

        o2pmap.push_back(boost::make_tuple(oid,pid,increasepid+pid));


        

    }
    odb->flush();
    last_odb->flush();
    boost::shared_ptr<OfferDbRecorder> odbr(new OfferDbRecorder(odb.get(), last_odb.get()));
    cout<<"OfferDbRecorder build"<<endl;
    odbr->open();
    cout<<"OfferDbRecorder open"<<endl;

    BOOST_CHECK_EQUAL(odbr->is_open(),true);

    for(std::vector<boost::tuple<uint128_t,uint128_t,uint128_t> >::iterator it=o2pmap.begin();it!=o2pmap.end();it++)
    {
        oid=(*it).get<0>();
        pid=(*it).get<1>();
        uint128_t lastpid=(*it).get<2>();
        uint128_t pidmirror,pidlast=0;
        bool ret=false;
        odbr->get(oid,pidmirror,ret);
        odbr->get_last(oid, pidlast);
        //std::cout<<"oid"<<oid<<"pid"<<pid<<"pidlast"<<pidlast<<"ret"<<ret<<std::endl;
        BOOST_CHECK_EQUAL(pid==pidmirror,true);
        BOOST_CHECK_EQUAL(pidlast==lastpid,true);
        BOOST_CHECK_EQUAL(ret^(pidmirror!=pidlast),false);
        


        //BOOST
        

    }
   
   
}


BOOST_AUTO_TEST_SUITE_END()

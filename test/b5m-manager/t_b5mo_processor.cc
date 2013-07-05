#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <b5m-manager/b5mo_processor.h>
#include "TestResources.h"
using namespace sf1r;
using namespace std;
using namespace boost;
//BOOST_AUTO_TEST_CASE()
//BOOST_AUTO_TEST_SUITE()



string toString(UString us)
{
    string str;
    us.convertString(str, izenelib::util::UString::UTF_8);
    return str;
}

void ProcessVector(ProductMatcher &matcher,vector<Document> docvec)
{
    ProductMatcher::Product result_product;
    for(unsigned i=0; i<docvec.size(); i++)
    {
        cout<<i<<endl;
        matcher.Process(docvec[i], result_product);
    }
}


string getTitle(Document doc)
{
    UString utitle;
    doc.getProperty("Title", utitle);
    return toString(utitle);
}

string getSource(Document doc)
{

    UString usource;
    doc.getProperty("Source", usource);
    return toString(usource);
}

string get(Document doc,string prop)
{

    UString ustr;
    doc.getProperty(prop, ustr);
    return toString(ustr);
}
void check(Document doc,string source,string price,string title,string category,string attrubute,string mobile)//,uint128_t docid,uint128_t uuid
{
    BOOST_CHECK_EQUAL(get(doc,"Source"),source);
    //  BOOST_CHECK_EQUAL(doc.property("DOCID")==docid,true);
    //  BOOST_CHECK_EQUAL(doc.property("uuid")==uuid,true);
    BOOST_CHECK_EQUAL(get(doc,"Price"),price);
    BOOST_CHECK_EQUAL(get(doc,"Title"),title);
    BOOST_CHECK_EQUAL(get(doc,"Category"),category);
    BOOST_CHECK_EQUAL(get(doc,"Attribute"),attrubute);
    BOOST_CHECK_EQUAL(doc.hasProperty("mobile"),mobile.length()>0);

}


void show(Document doc)
{
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<"  mobile "<<doc.property("mobile") <<endl;
}
void check(ProductMatcher::Product product,string fcategory,string scategory,string sbrand)
{
    BOOST_CHECK_EQUAL(product.scategory.find(scategory)==string::npos,false);
    BOOST_CHECK_EQUAL(product.fcategory.find(fcategory)==string::npos,false);
    BOOST_CHECK_EQUAL(product.sbrand,sbrand);
}


void show(ProductMatcher::Product product)
{
    cout<<"product"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue()<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}


BOOST_AUTO_TEST_SUITE(b5mo_processor_test)

BOOST_AUTO_TEST_CASE(b5mo_processor_process)
{
    string bdb_path="./bdb";
    string odb_path="./odb";
    std::string cma_path= IZENECMA_KNOWLEDGE ;

    //string scd_path="/home/lscm/6indexSCD";
    string output_dir="./output";

    boost::filesystem::remove_all(bdb_path);
    boost::filesystem::remove_all(odb_path);
    //b5mo process
    boost::shared_ptr<BrandDb> bdb;
    boost::shared_ptr<OfferDb> odb;
    bdb.reset(new BrandDb(bdb_path));
    odb.reset(new OfferDb(odb_path));
    odb->open();
    bdb->open();
    int mode=0;
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);


    uint128_t pid=B5MHelper::StringToUint128("5f29098f1f606d9daeb41e49e9a24f87");
    uint128_t docid=1;
    Document::doc_prop_value_strtype brand("联想");
    bdb->set(pid, brand);

    Document doc;
    SCD_TYPE type=INSERT_SCD;
    doc.property("Title") = str_to_propstr("苹果 iphone4s", UString::UTF_8);
    doc.property("DOCID") = str_to_propstr(B5MHelper::Uint128ToString(docid), UString::UTF_8);
    doc.property("Price") = str_to_propstr("100.0", UString::UTF_8);
    doc.property("Source") = str_to_propstr("京东", UString::UTF_8);
    doc.property("Attribute") = str_to_propstr("品牌:苹果", UString::UTF_8);
    odb->insert(1, pid);
    odb->flush();
    odb->get(docid, pid);
    B5moProcessor processor(odb.get(), matcher, bdb.get(), mode, NULL);


    processor.Process(doc,type);

    check(doc,"京东","100.00","苹果 iphone4s","","品牌:苹果","");//1,0x05f29098f1f606d9daeb41e49e9a24f87,"
    uint128_t bdocid;

    bdocid=B5MHelper::StringToUint128(get(doc,"DOCID"));
    bdb->get(bdocid, brand);
    BOOST_CHECK_EQUAL( propstr_to_str(brand),"联想");
    doc.property("Category") = str_to_propstr("平板电脑/MID", UString::UTF_8);
    type=UPDATE_SCD;
    doc.property("Source") = str_to_propstr("天猫", UString::UTF_8);
    doc.property("Price") = str_to_propstr("106.0", UString::UTF_8);
    processor.Process(doc,type);
    check(doc,"天猫","106.00","苹果 iphone4s", "平板电脑/MID","品牌:苹果","");//1,0x5f29098f1f606d9daeb41e49e9a24f87, "


    type=INSERT_SCD;
    Document doc2;
    docid=2;
    doc2.property("Source") = str_to_propstr("天猫", UString::UTF_8);
    doc2.property("DOCID") = str_to_propstr(B5MHelper::Uint128ToString(docid), UString::UTF_8);
    doc2.property("Price") = str_to_propstr("154.0", UString::UTF_8);
    doc2.property("Category") = str_to_propstr("手机", UString::UTF_8);
    doc2.property("Attribute") = str_to_propstr("品牌:三星", UString::UTF_8);
    odb->insert(2, pid);
    odb->flush();
    processor.Process(doc2,type);
    check(doc2,"天猫","154.00","","手机","品牌:三星","");
   

    type=DELETE_SCD;
    processor.Process(doc,type);
    bdb->get(bdocid, brand);
    BOOST_CHECK_EQUAL(toString(brand),"联想");
    Document doc3;
    type=INSERT_SCD;
    doc3.property("Source") = str_to_propstr("当当", UString::UTF_8);
    doc3.property("DOCID") = str_to_propstr("03", UString::UTF_8);
    doc3.property("Price") = str_to_propstr("15434.0", UString::UTF_8);
    doc3.property("Category") = str_to_propstr("男装>夹克", UString::UTF_8);
    doc3.property("Attribute") = str_to_propstr("品牌:brand1", UString::UTF_8);
    processor.Process(doc3,type);
//当当 03 03 15434.00    男装>夹克 品牌:brand1  mobile

    check(doc3,"当当","15434.00","","男装>夹克","品牌:brand1","");
    processor.Process(doc,type);
    Document doc4;
    doc4.property("Title") = str_to_propstr("SAMSUNG/三星 GALAXYTAB2 P3100 8GB 3G版");
    doc4.property("DOCID") = str_to_propstr("04", UString::UTF_8);
    doc4.property("Price") = str_to_propstr("1324.0", UString::UTF_8);
    doc4.property("Category") = str_to_propstr("平板电脑/MID");
    type=INSERT_SCD;
    processor.Process(doc,type);


}


BOOST_AUTO_TEST_SUITE_END()



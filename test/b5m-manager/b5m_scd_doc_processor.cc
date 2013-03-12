#include <b5m-manager/raw_scd_generator.h>
#include <b5m-manager/attribute_indexer.h>
#include <b5m-manager/product_matcher.h>
#include <b5m-manager/category_scd_spliter.h>
#include <b5m-manager/b5mo_scd_generator.h>
#include <b5m-manager/b5mo_processor.h>
#include <b5m-manager/log_server_client.h>
#include <b5m-manager/image_server_client.h>
#include <b5m-manager/uue_generator.h>
#include <b5m-manager/complete_matcher.h>
#include <b5m-manager/similarity_matcher.h>
#include <b5m-manager/ticket_matcher.h>
#include <b5m-manager/ticket_processor.h>
#include <b5m-manager/uue_worker.h>
#include <b5m-manager/b5mp_processor.h>
#include <b5m-manager/spu_processor.h>
#include <b5m-manager/b5m_mode.h>
#include <b5m-manager/b5mc_scd_generator.h>
#include <b5m-manager/log_server_handler.h>
#include <b5m-manager/product_db.h>
#include <b5m-manager/offer_db.h>
#include <b5m-manager/offer_db_recorder.h>
#include <b5m-manager/brand_db.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/history_db_helper.h>
#include <b5m-manager/psm_indexer.h>
#include <b5m-manager/cmatch_generator.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <stack>
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/decode.hpp>
#include <b5m-manager/scd_doc_processor.h>
#include <b5m-manager/product_db.h>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <util/test/BoostTestThreadSafety.h>

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





int main()
{
    string bdb_path="./bdb";
    string odb_path="./odb";
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir= MATCHER_KNOWLEDGE;
    //string scd_path="/home/lscm/6indexSCD";
    string output_dir="./output";
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";

    //b5mo process
    boost::shared_ptr<BrandDb> bdb;
    boost::shared_ptr<OfferDb> odb;
    bdb.reset(new BrandDb(bdb_path));
    odb.reset(new OfferDb(odb_path));
    odb->open();
    bdb->open();
    boost::shared_ptr<ProductMatcher> matcher(new ProductMatcher);
    matcher->SetCmaPath(cma_path);
    if(!matcher->Open(knowledge_dir))
    {

    }
    int mode=0;



    uint128_t pid=B5MHelper::StringToUint128("5f29098f1f606d9daeb41e49e9a24f87");
    uint128_t docid=1;
    UString brand("联想",izenelib::util::UString::UTF_8);
    bdb->set(pid, brand);



    Document doc;
    SCD_TYPE type=INSERT_SCD;
    doc.property("Title") = UString("苹果 iphone4s", UString::UTF_8);
    doc.property("DOCID") = UString(B5MHelper::Uint128ToString(docid), UString::UTF_8);
    doc.property("Price") =UString("100.0", UString::UTF_8);
    doc.property("Source") =UString("京东", UString::UTF_8);
    doc.property("Attribute") =UString("品牌:苹果", UString::UTF_8);
    odb->insert(1, pid);
    odb->flush();
    odb->get(docid, pid);
    B5moProcessor processor(odb.get(), matcher.get(), bdb.get(), mode, NULL);

    processor.LoadMobileSource(file);
    processor.Process(doc,type);

    check(doc,"京东","100.00","苹果 iphone4s","平板电脑/MID","品牌:苹果,型号:P85,容量:16G/16GB","1");//1,0x05f29098f1f606d9daeb41e49e9a24f87,"
    uint128_t bdocid;

    bdocid=B5MHelper::StringToUint128(get(doc,"DOCID"));
    bdb->get(bdocid, brand);
    BOOST_CHECK_EQUAL(toString(brand),"联想");
    type=UPDATE_SCD;
    doc.property("Source") =UString("天猫", UString::UTF_8);
    doc.property("Price") =UString("106.0", UString::UTF_8);
    processor.Process(doc,type);
    check(doc,"天猫","106.00","苹果 iphone4s", "平板电脑/MID","品牌:苹果,型号:P85,容量:16G/16GB","1");//1,0x5f29098f1f606d9daeb41e49e9a24f87, "


    type=INSERT_SCD;
    Document doc2;
    docid=2;
    doc2.property("Source") =UString("天猫", UString::UTF_8);
    doc2.property("DOCID") = UString(B5MHelper::Uint128ToString(docid), UString::UTF_8);
    doc2.property("Price") =UString("154.0", UString::UTF_8);
    doc2.property("Category") =UString("手机", UString::UTF_8);
    doc2.property("Attribute") =UString("品牌:三星", UString::UTF_8);
    odb->insert(2, pid);
    odb->flush();
    processor.Process(doc2,type);
    check(doc2,"天猫","154.00","","手机","品牌:三星","");
    ProductMatcher::Product product;
    matcher->GetProduct(get(doc2,"uuid"), product);
    //check(product);

    type=DELETE_SCD;
    processor.Process(doc,type);
    bdb->get(bdocid, brand);
    BOOST_CHECK_EQUAL(toString(brand),"联想");
    Document doc3;
    type=INSERT_SCD;
    doc3.property("Source") =UString("当当", UString::UTF_8);
    doc3.property("DOCID") = UString("03", UString::UTF_8);
    doc3.property("Price") =UString("15434.0", UString::UTF_8);
    doc3.property("Category") =UString("男装>夹克", UString::UTF_8);
    doc3.property("Attribute") =UString("品牌:brand1", UString::UTF_8);
    processor.Process(doc3,type);
//当当 03 03 15434.00    男装>夹克 品牌:brand1  mobile

    check(doc3,"当当","15434.00","","男装>夹克","品牌:brand1","");
    processor.Process(doc,type);
    Document doc4;
    doc4.property("Title") ="SAMSUNG/三星 GALAXYTAB2 P3100 8GB 3G版";
    doc4.property("DOCID") = UString("04", UString::UTF_8);
    doc4.property("Price") =UString("1324.0", UString::UTF_8);
    doc4.property("Category") ="平板电脑/MID";
    type=INSERT_SCD;
    processor.Process(doc,type);


//    mathcher process using namespace ProductMatcher;
    std::vector<ProductMatcher::Product> result_products;


    string spid="7bc999f5d10830d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="苏宁",date="20130301",price="1043",attribute="产地:中国,质量:优",title="华硕  TF700T";
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(title, izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);
    matcher->Process(doc, 3, result_products);

    for(unsigned i=0; i<result_products.size(); i++)
    {
        check(result_products[i],"电脑办公>电脑整机>笔记本","笔记本电脑","Asus/华硕");
    }
    std::vector<ProductMatcher::Product> result_products2;
    spid="72c999f5d10830d0c59487bd48a73cae",soid="35c999f5d10830d0c59487bd48adce8a",source="凡客",date="20130301",price="1043",attribute="产地:韩国,质量:差",title="2012秋冬新款女外精品棉大衣";
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(title, izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);
    matcher->Process(doc, 3, result_products2);



    for(unsigned i=0; i<result_products2.size(); i++)
    {
        check(result_products2[i],"服饰鞋帽>女装","女装/女士精品","");
    }

}




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
using namespace sf1r;
using namespace std;
using namespace boost;

string toString(UString us)
{
    string str;
    us.convertString(str, izenelib::util::UString::UTF_8);
    return str;
}
//BOOST_AUTO_TEST_CASE()
//BOOST_AUTO_TEST_SUITE()
void Process()
{
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

void show(Document doc)
{
     cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<get(doc,"品牌")<<  doc.property(B5MHelper::GetBrandPropertyName()) <<"  mobile "<<doc.property("mobile") <<endl;
}

void show(ProductMatcher::Product product)
{
    cout<<"product"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue()<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}


int main()
{
    string bdb_path="/home/lscm/codebase/b5m-operation/db/working_b5m/knowledge/bdb";
    string odb_path="/home/lscm/codebase/b5m-operation/db/working_b5m/db/mdb/odbnew";
    string cma_path="/home/lscm/codebase/icma/db/icwb/utf8";
    string knowledge_dir="/home/lscm/codebase/b5m-operation/db/working_b5m/knowledge/";
    string scd_path="/home/lscm/6indexSCD";
    string output_dir="./output";

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
            return EXIT_FAILURE;
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
    bool doesget=odb->get(docid, pid);
    cout<<"get"<<doesget<<endl;
    B5moProcessor processor(odb.get(), matcher.get(), bdb.get(), mode, NULL);
    string file="/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    processor.LoadMobileSource(file);
    cout<<"pid"<<pid<<"docid"<<docid<<endl;  
    cout<<"process"<<endl;
    processor.Process(doc,type);
    cout<<"process over"<<endl;
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<< doc.property(B5MHelper::GetBrandPropertyName()) <<"  mobile "<<doc.property("mobile") <<endl;

    uint128_t bdocid;
    cout<<get(doc,"DOCID")<<endl;
    bdocid=B5MHelper::StringToUint128(get(doc,"DOCID"));
    cout<<"docid"<<bdocid<<endl;
    bdb->get(bdocid, brand);
    cout<<"brand"<<toString(brand)<<endl;
    type=UPDATE_SCD;
    doc.property("Source") =UString("天猫", UString::UTF_8);   
    doc.property("Price") =UString("106.0", UString::UTF_8);
    processor.Process(doc,type);
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<get(doc,"品牌")<<  doc.property(B5MHelper::GetBrandPropertyName()) <<"  mobile "<<doc.property("mobile") <<endl;


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
    
    cout<<get(doc2,"Source")<<" "<<doc2.property("DOCID")<<" "<<doc2.property("uuid")<<" "<<get(doc2,"Price")<<"  "<<get(doc2,"Title")<<"  "<<get(doc2,"Category")<<" "<<get(doc2,"Attribute")<<get(doc2,"品牌")<<"  mobile "<<doc2.property("mobile") <<endl;
    ProductMatcher::Product product;
    matcher->GetProduct(get(doc2,"uuid"), product);
    cout<<"product"<<get(doc2,"uuid")<<"attributes"<<product.GetAttributeValue()<<"category"<<product.fcategory<<"title"<<product.stitle<<endl;

    type=DELETE_SCD;
    processor.Process(doc,type);
    bdb->get(bdocid, brand);
    cout<<"brand"<<toString(brand)<<endl;//
   
    Document doc3;
    type=INSERT_SCD;
    doc3.property("Source") =UString("当当", UString::UTF_8);
    doc3.property("DOCID") = UString("03", UString::UTF_8);
    doc3.property("Price") =UString("15434.0", UString::UTF_8);
    doc3.property("Category") =UString("男装>夹克", UString::UTF_8);
    doc3.property("Attribute") =UString("品牌:brand1", UString::UTF_8);
    processor.Process(doc3,type);
    cout<<get(doc3,"Source")<<" "<<doc3.property("DOCID")<<" "<<doc3.property("uuid")<<" "<<get(doc3,"Price")<<"  "<<get(doc3,"Title")<<"  "<<get(doc3,"Category")<<" "<<get(doc3,"Attribute")<<get(doc3,"品牌")<<"  mobile "<<doc3.property("mobile") <<endl;
    type=DELETE_SCD;
    processor.Process(doc,type);
    Document doc4;
    doc4.property("Title") ="SAMSUNG/三星 GALAXYTAB2 P3100 8GB 3G版";
    doc4.property("DOCID") = UString("04", UString::UTF_8);
    doc3.property("Price") =UString("1324.0", UString::UTF_8);
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

    cout<<"result_products"<<result_products.size()<<endl;
    for(int i=0;i<result_products.size();i++)
    {
       show(result_products[i]);
    }

    spid="72c999f5d10830d0c59487bd48a73cae",soid="35c999f5d10830d0c59487bd48adce8a",source="凡客",date="20130301",price="1043",attribute="产地:韩国,质量:差",title="2012秋冬新款女外套 韩版胖mm宽松加大码中长款棉衣泡泡袖棉服";
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(title, izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);
    matcher->Process(doc, 3, result_products);
    cout<<"result_products"<<result_products.size()<<endl;
    for(int i=0;i<result_products.size();i++)
    {
       show(result_products[i]);
    }

//   ScdDocProcessor


    ScdDocProcessor::ProcessorType p = boost::bind(&Process);
    ScdDocProcessor sd_processor(p);
    sd_processor.AddInput(scd_path);
    B5MHelper::PrepareEmptyDir(output_dir);
    sd_processor.SetOutput(output_dir);
    sd_processor.Process();
/**/
}


//BOOST_AUTO_TEST_SUITE_END()


#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>
#include "../TestResources.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/product_matcher.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include "TestResources.h"
using namespace sf1r;
using namespace std;
using namespace boost;


string toString(UString us)
{
    string str;
    us.convertString(str, izenelib::util::UString::UTF_8);
    return str;
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
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<"  mobile "<<doc.property("mobile") <<endl;
}


void show(ProductMatcher::Product product)
{
    cout<<"product"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue()<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}

bool checkProduct(ProductMatcher::Product product,string fcategory,string scategory,string sbrand)
{
    if(product.scategory.find(scategory)!=string::npos)return false;
    if(product.fcategory.find(fcategory)!=string::npos)return false;
    if(product.sbrand!=sbrand)return false;
    return true;
}
bool checkProcesss(ProductMatcher &matcher,string spid,string soid,string source,string date,string price,string attribute,string title,string fcategory,string scategory,string sbrand)
{
    Document doc;
    doc.property("DATE") = izenelib::util::UString(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(spid,izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(title, izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(attribute,izenelib::util::UString::UTF_8);

    std::vector<ProductMatcher::Product> result_products;
    matcher.Process(doc, 3, result_products);
    for(unsigned i=0; i<result_products.size(); i++)
    {
        bool ret=checkProduct(result_products[i],fcategory, scategory, sbrand);
        if(!ret)
        {
            show(doc);
            show(result_products[i]);
        }
    }

}
void write(Document  doc,ProductMatcher::Product product)
{
    ofstream out;
    out.open("write",ios::app|ios::out);
    out<<"checkProcesss(matcher,\""<<doc.property("uuid")<<"\",\""<< doc.property("DOCID")<<"\",\""<< get(doc,"Source")<<"\",\""<< get(doc,"DATE")<<"\",\""<< get(doc,"Price")<<"\",\""<< get(doc,"Attribute")<<"\",\""<< get(doc,"Title")<<"\",\""<< product.fcategory<<"\",\""<<  product.scategory<<"\",\""<<  product.sbrand<<"\");"<<endl;
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
/*
void MultiThreadProcessTest()
{
    unsigned threadnum=4;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    std::vector<ProductMatcher*> matcher;
    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    for(unsigned i=0; i<threadnum; i++)
    {
        matcher.push_back(new ProductMatcher);
        matcher[i]->SetCmaPath(cma_path);
        bool ret=matcher[i]->Open(knowledge_dir);
        BOOST_CHECK_EQUAL(ret,true);
    }
    //boost::thread_group threads;
    vector<vector<Document> > threaddocvec;
    threaddocvec.resize(threadnum);
    string scd_file= TEST_SCD_PATH;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10000==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>40000)
        {
            break;
        }
        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();

        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
        }

        threaddocvec[n%threadnum].push_back(doc);

    }

    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&ProcessVector,matcher,threaddocvec[i])));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    //threads.join_all();
    for(unsigned i=0; i<threadnum; i++)
    {
        delete matcher[i];
    }

};
*/
int main(int ac, char** av)
{
   
    cout<<"here!"<<endl;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string scd_file;//= TEST_SCD_PATH;
    
    if(ac==4)
    {
       //LOG(INFO)<<"the command should be like this:   ./b5m_product_matcher  knowledge_dir  cma_path  scd_path";
       knowledge_dir=av[1];
       cma_path=av[2];
       scd_path=av[3];
    }
    else
    {
       cout<<"the command should be like this:   ./b5m_product_matcher  knowledge_dir  cma_path  scd_path"<<endl;
       return 0;
    }
    
    bool noprice=false;
    int max_depth=3;

    if( knowledge_dir.empty())
    {
    }
    ProductMatcher matcher;
    matcher.SetCmaPath(cma_path);
    if(!matcher.Open(knowledge_dir))
    {
        LOG(ERROR)<<"matcher open failed"<<std::endl;
    }
    if(noprice)
    {
        matcher.SetUsePriceSim(false);
    }
    if(max_depth>0)
    {
        matcher.SetCategoryMaxDepth(max_depth);
    }

    vector<std::pair<string,string> >  txt;
    txt.push_back(make_pair("限时包邮2012春装新款大摆半身裙波西米亚长裙夏季蓬蓬裙网纱裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("秋冬新款欧美风厚实喇叭大裙摆黑灰裙裤打底裤显瘦裤裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("韩国代购stylenanda左右不对称黑色长裙","服饰鞋帽>女装>裙子"));
    txt.push_back(make_pair("七匹狼运动专卖店,七匹狼羽绒服 可卸帽高档保暖白鸭绒外套男 冬装特价运动男装,七匹狼,羽绒服,可卸帽,高档,保暖,鸭绒,外套,冬装,特价,运动,男装 ","服饰鞋帽>男装>羽绒服"));
    txt.push_back(make_pair("IBM E420 E420(1141A86) Thinkpad 新品促销 ","电脑办公>电脑整机>笔记本"));
    txt.push_back(make_pair("大陆行货 Apple/苹果 IPHONE 4 (8G) iPhone4 全新正品 全国联保 ","手机数码>手机通讯>手机"));
    txt.push_back(make_pair("孝傲江湖 秋季新款女鞋高品质亮丽漆皮女时尚休闲女鞋蝴蝶结布鞋175222 黑色 35 ","服饰鞋帽>女鞋>布鞋"));


    for(unsigned i=0; i<txt.size(); i++)
    {
        //cout<<txt[i]<<"  ";
        std::vector<UString> results;
        UString text(txt[i].first, UString::UTF_8);
        matcher.GetFrontendCategory(text, 3, results);
        //cout<<"result:"<<results.size();
        bool get=false;
        for(unsigned j=0; j<results.size(); j++)
        {
            string str;
            results[j].convertString(str, UString::UTF_8);
            //cout<<str;
            if(str==txt[i].second)
            {
                get=true;
            }
        }
        if(!get)
        {
             std::cout<<"error"<<txt[i].first<<txt[i].second<<"result:"<<results.size()<<std::endl;
        }
        //cout<<endl;
    }



    checkProcesss(matcher,"7bc999f5d10830d0c59487bd48a73cae","46c999f5d10830d0c59487bd48adce8a","苏宁","20130301","1043","产地:中国,质量:优","华硕  TF700T","电脑办公>电脑整机>笔记本","笔记本电脑","Asus/华硕");
    checkProcesss(matcher,"72c999f5d10830d0c59487bd48a73cae","35c999f5d10830d0c59487bd48adce8a","凡客","20130301","1043","产地:韩国,质量:差","2012秋冬新款女外精品棉大衣","服饰鞋帽>女装","女装/女士精品","");

    std::cout<<"here!"<<endl;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>80000)
        {
            break;
        }
        

        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();

        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
        }
        if(n%1000==0)
        {
        //threaddocvec[n%threadnum].push_back(doc);
        ProductMatcher::Product result_product;

        

        matcher.Process(doc, result_product); 
        write(doc,result_product);
        }

    }
    return 1;
};


/*

BOOST_AUTO_TEST_CASE(Multithread_case)
{
    unsigned threadnum=4;
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    std::vector<ProductMatcher*> matcher;
    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    for(unsigned i=0; i<threadnum; i++)
    {
        matcher.push_back(new ProductMatcher);
        matcher[i]->SetCmaPath(cma_path);
        bool ret=matcher[i]->Open(knowledge_dir);
        BOOST_CHECK_EQUAL(ret,true);
    }
    //boost::thread_group threads;
    vector<vector<Document> > threaddocvec;
    threaddocvec.resize(threadnum);
    string scd_file= TEST_SCD_PATH;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10000==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>40000)
        {
            break;
        }
        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();

        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
        }

        threaddocvec[n%threadnum].push_back(doc);

    }

    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&Proce)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    //threads.join_all();
    for(unsigned i=0; i<threadnum; i++)
    {
        delete matcher[i];
    }

}
*/



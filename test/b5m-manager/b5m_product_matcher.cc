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

unsigned getCategoryError=0;
unsigned ProcessError=0;
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
/*
string getId(Document doc,string prop)
{
    uint128_t id;
    doc.getProperty(prop, id);
    return boost::lexical_cast<int>(id);
}
*/
void show(Document doc)
{
    cout<<get(doc,"Source")<<" "<<doc.property("DOCID")<<" "<<doc.property("uuid")<<" "<<get(doc,"Price")<<"  "<<get(doc,"Title")<<"  "<<get(doc,"Category")<<" "<<get(doc,"Attribute")<<"  mobile "<<doc.property("mobile") <<endl;
}


void show(ProductMatcher::Product product)
{
   cout<<"productid"<<product.spid<<"title"<< product.stitle<<"attributes"<<product.GetAttributeValue()<<"frontcategory"<<product.fcategory<<"scategory"<<product.scategory<<"price"<<product.price<<"brand"<<product.sbrand<<endl;
}

bool checkProduct(ProductMatcher::Product &product,string &fcategory,string &scategory,string &sbrand,string spid)
{
    if(product.scategory.find(scategory)==string::npos)return false;
    if(product.fcategory.find(fcategory)==string::npos)return false;
    if(product.sbrand!=sbrand)return false;
    if(product.spid!=spid)return false;
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

    ProductMatcher::Product result_product;
    matcher.Process(doc,result_product,true);
    bool check=true;

    {
        bool ret=checkProduct(result_product,fcategory, scategory, sbrand,spid);
        if(!ret)
        {
            show(doc);
            show(result_product);
            check=false;
        }
    }
    if(!check)
    {
        ProcessError++;
    }
    return check;
}


void ProcessVector(ProductMatcher* matcher,vector<Document>& docvec, vector<ProductMatcher::Product>& result_product)
{
    result_product.resize(docvec.size());
    for(unsigned i=0; i<docvec.size(); i++)
    {
        matcher->Process(docvec[i], result_product[i],true);
    }
}


void CheckProcessVector(ProductMatcher* matcher,vector<Document>& doc,  vector<ProductMatcher::Product>& product)
{
    ProductMatcher::Product re;
    for(unsigned i=0; i<doc.size(); i++)
    {

        checkProcesss((*matcher),product[i].spid, "",get(doc[i],"Source"), get(doc[i],"DATE"), get(doc[i],"Price"), get(doc[i],"Attribute"), get(doc[i],"Title"),product[i].fcategory,  product[i].scategory, product[i].sbrand);
    }
}



void MultiThreadProcessTest(string scd_file, std::string knowledge_dir,    unsigned threadnum=4,unsigned docnum=40000)
{
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    //std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    //std::vector<ProductMatcher*> matcher;
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    /*
    for(unsigned i=0; i<threadnum; i++)
    {
        matcher.push_back(new ProductMatcher);
        matcher[i]->SetCmaPath(cma_path);
        matcher[i]->Open(knowledge_dir);

    }
    */
    //boost::thread_group threads;
    vector<vector<Document> > threaddocvec;

    vector<Document> right;
    threaddocvec.resize(threadnum);

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
            doc_iter!= parser.end(); ++doc_iter, ++n)
    {

        if(n%10==0)
        {
            //LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        if(n>docnum)
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
    vector<vector<ProductMatcher::Product> > Product;
    Product.resize(threadnum);
    //single thread to get right answer
    for(unsigned i=0; i<threadnum; i++)
    {
         ProcessVector(matcher,threaddocvec[i],Product[i]);

    }
    // multithread thread for check
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&CheckProcessVector,matcher,threaddocvec[i],Product[i])));
        //compute_threads_[i]=(new boost::thread(boost::bind(&Process)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    //threads.join_all();

    

    
    
    delete matcher;


};

void SegLine(std::string temp, vector<string>& ret)
{
    ret.clear();
    temp=temp.substr(temp.find("\"")+1);
    temp=temp.substr(0,temp.size()-1);
    size_t templen = temp.find("\",\"");
    while(templen!= string::npos)
    {

        
        ret.push_back(temp.substr(0,templen));
        
       
        temp=temp.substr(templen+3);
        templen = temp.find("\",\"");

    }
  
    ret.push_back(temp);
    
}
int main()
{

    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir;//= MATCHER_KNOWLEDGE;
    string scd_file;//= TEST_SCD_PATH;
    string category_test_file;//
    string process_test_file;//
    ifstream in;
    in.open("config",ios::in);
    string line;
    unsigned threadnum=4;
    while( getline(in, line))
    {
        if(line.find("knowledge_dir:")!=string::npos){knowledge_dir=line.substr(line.find("knowledge_dir:")+string("knowledge_dir:").length());}
        else if(line.find("scd_path:")!=string::npos){scd_file=line.substr(line.find("scd_path:")+string("scd_path:").length());}
        else if(line.find("category_test_file")!=string::npos){category_test_file=line.substr(line.find("category_test_file:")+string("category_test_file:").length());}
        else if(line.find("process_test_file")!=string::npos){process_test_file=line.substr(line.find("process_test_file:")+string("process_test_file:").length());}
        else if(line.find("threadnum")!=string::npos){threadnum=boost::lexical_cast<int>(line.substr(line.find("threadnum:")+string("threadnum:").length()));}

    }
    cout<< cma_path<<"  "<<knowledge_dir<<" "<<scd_file<<" "<<category_test_file<<" "<<process_test_file<<endl;
    in.close();
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

    in.open(category_test_file.c_str(),ios::in);
    string title;
    string category;
    int num=0;
    while( getline(in, line))
    {
         num++;
         if(num%2==1)
         title=line;
         else
         {
            category=line;
            txt.push_back(make_pair(title,category));
         }
    }

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
            if(str.find(txt[i].second)!=string::npos||txt[i].second.find(str)!=string::npos)
            {
                get=true;
            }
        }
        if(!get)
        {
            std::cout<<"error"<<txt[i].first<<"RightAnswer"<<txt[i].second<<"size"<<results.size();
            for(unsigned j=0; j<results.size(); j++)
            {
                string str;
                results[j].convertString(str, UString::UTF_8);
                cout<<str;
            }
            getCategoryError++;
            cout<<endl;
        }
        //cout<<endl;
    }
    cout<<"错误率："<<double(getCategoryError)/txt.size()<<endl;
    sleep(10);
int DocNum=0;
in.close();
if(!process_test_file.empty())
{
    in.open(process_test_file.c_str(),ios::in);
    string title;
    string category;

    vector<string> param;
    while( getline(in, line))
    {
         //cout<<line<<endl;
         SegLine(line,param);
         /*
         cout<<param.size()<<" ";
         for(unsigned j=0;j<param.size();j++)
         {
              cout<<param[j]<<" ";
         }
         cout<<endl;
         */
         if(param.size()==10)
         {
           checkProcesss(matcher,param[0],param[1],param[2],param[3],param[4],param[5],param[6],param[7],param[8],param[9]);
           DocNum++;
         }
    }


}
/*    matcher.Test("/home/lscm/6indexSCD/");
*/
    cout<<DocNum<<endl;
    if(DocNum!=0)
    {
    cout<<"错误率："<<double(ProcessError)/DocNum<<endl;
    }
    sleep(10);
    MultiThreadProcessTest(scd_file,knowledge_dir,threadnum);

    cout<<"错误率："<<double(ProcessError)/(40000+DocNum)<<endl;

    return 1;
};



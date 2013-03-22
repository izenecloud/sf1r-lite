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
UCS2Char getUcs2Char(int iRange1 ,int iRange2)
{
    return rand()%(iRange2-iRange1) +iRange1;
}
string rand_chinese_char()
{
    string str = "一";
    UString text(str, UString::UTF_8);
    switch (rand()%5)
    {
    case 0:
        text[0]=getUcs2Char(0x2E80,0x2EF3);
        break;

    case 1:
        text[0]=getUcs2Char(0x2F00 ,0x4DB5);
        break;
    case 2:
        text[0]=getUcs2Char(0x3400 ,0x4DB5);
        break;

    case 3:
        text[0]=getUcs2Char(0x4E00 ,0x9FC3);
        break;

    default:
        text[0]=getUcs2Char(0xF900,0xFAD9);
        break;
    }
    return toString(text);
}
string rand_korea_char()
{
    string str = "왜";
    UString text(str, UString::UTF_8);
    switch (rand()%2)
    {
    case 0:
        text[0]=getUcs2Char(0x1100,0x11ff);
        break;

    case 1:
        text[0]=getUcs2Char(0xAC00,0xD7AF);
        break;

    }
    return toString(text);

}
string rand_jp_char()
{
    string str = "一";
    UString text(str, UString::UTF_8);
    switch (rand()%4)
    {
    case 0:
        text[0]=getUcs2Char(0x3041,0x309F);
        break;

    case 1:
        text[0]=getUcs2Char(0x30A1 ,0x30FF);
        break;
    case 2:
        text[0]=getUcs2Char(0x31F0 ,0x31FF);
        break;

    case 3:
        text[0]=getUcs2Char(0xFF66,0xFF9F);
        break;

    }
    return toString(text);
}
const string en_char = "_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
string rand_en_str()
{

    string ret;
    int size= rand()%5+1;
    for (int i = 0; i <size; ++i)
    {
        int x =rand() %(en_char.size()-1);
        //cout<<en_char.substr(x,1)<<endl;
        ret = ret+en_char.substr(x,1);
    }
    return ret;
}
string rand_str()
{
    int len=rand()%100;
    int i1;
    string a;
    for(i1=1; i1<len; ++i1)
    {
        switch (rand()%4)
        {
        case 0:
            a+= rand_en_str();
            break;

        case 1:
            a+= rand_jp_char();
            break;
        case 2:
            a+= rand_korea_char();
            break;

        case 3:
            a+= rand_chinese_char();
            break;

        }
    }

    return a;
}
string rand_id()
{
    return  B5MHelper::Uint128ToString(rand());
}
Document rand_doc()
{
    Document doc;
    doc.property("DATE") = izenelib::util::UString(rand_str(), izenelib::util::UString::UTF_8);
    doc.property("DOCID") =     izenelib::util::UString(rand_id(),izenelib::util::UString::UTF_8);
    doc.property("Title") = izenelib::util::UString(rand_str(), izenelib::util::UString::UTF_8);
    doc.property("Price") = izenelib::util::UString(rand_str(),izenelib::util::UString::UTF_8);
    doc.property("Source") = izenelib::util::UString(rand_str(),izenelib::util::UString::UTF_8);
    doc.property("Attribute") = izenelib::util::UString(rand_str(),izenelib::util::UString::UTF_8);
    return doc;
}

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
void GetFrontEnd(ProductMatcher* matcher,  boost::posix_time::ptime time_end)
{
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time(); ;
    while( time_now <time_end)
    {
        std::vector<UString> results;
        UString text(rand_str(), UString::UTF_8);
        matcher->GetFrontendCategory(text, 3, results);
        time_now = boost::posix_time::microsec_clock::local_time();
    }
}
void SearchKeywords(ProductMatcher* matcher,  boost::posix_time::ptime time_end)
{
    boost::posix_time::ptime time_now  = boost::posix_time::microsec_clock::local_time();;
    while( time_now <time_end)
    {
        UString text(rand_str(), UString::UTF_8);
        std::list<std::pair<UString, double> > hits;
        std::list<UString> left;
        matcher->GetSearchKeywords(text, hits, left);
    }
}

void ProcessRandom(ProductMatcher* matcher,  boost::posix_time::ptime time_end)
{
    boost::posix_time::ptime time_now  = boost::posix_time::microsec_clock::local_time();;
    while( time_now <time_end)
    {
        ProductMatcher::Product result_product;
        matcher->Process(rand_doc(), result_product,true);
        time_now = boost::posix_time::microsec_clock::local_time();
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

void MultiThreadGetCategoryTest(string scd_file, std::string knowledge_dir, int minute,   unsigned threadnum=4,unsigned docnum=40000)
{
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    vector<vector<Document> > threaddocvec;

    vector<Document> right;
    threaddocvec.resize(threadnum);

    boost::posix_time::ptime time_end=boost::posix_time::microsec_clock::local_time()+boost::posix_time::minutes(minute);
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&GetFrontEnd,matcher,time_end)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }

    delete matcher;


};


void MultiThreadSearchKeywordsTest(string scd_file, std::string knowledge_dir, int minute,   unsigned threadnum=4,unsigned docnum=40000)
{
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    //std::string knowledge_dir= MATCHER_KNOWLEDGE;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);
    vector<vector<Document> > threaddocvec;

    vector<Document> right;
    threaddocvec.resize(threadnum);

    boost::posix_time::ptime time_end=boost::posix_time::microsec_clock::local_time()+boost::posix_time::minutes(minute);
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&GetFrontEnd,matcher,time_end)));

    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }

    delete matcher;
};



void MultiThreadRandomProcessTest(string scd_file, std::string knowledge_dir, int minute,   unsigned threadnum=4,unsigned docnum=40000)
{
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);

    vector<vector<Document> > threaddocvec;

    vector<Document> right;
    threaddocvec.resize(threadnum);
    boost::posix_time::ptime time_end=boost::posix_time::microsec_clock::local_time()+boost::posix_time::minutes(minute);

    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]=(new boost::thread(boost::bind(&ProcessRandom,matcher,time_end)));
    }
    cout<<"preadd"<<endl;
    for(unsigned i=0; i<threadnum; i++)
    {
        compute_threads_[i]->join();
    }
    delete matcher;


};

void MultiThreadProcessTest(string scd_file, std::string knowledge_dir,    unsigned threadnum=4,unsigned docnum=40000)
{
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    string file=MOBILE_SOURCE;//"/home/lscm/codebase/b5m-operation/config/collection/mobile_source.txt";
    ProductMatcher*  matcher=(new ProductMatcher);;
    matcher->SetCmaPath(cma_path);
    matcher->Open(knowledge_dir);

    std::vector<boost::thread*>  compute_threads_;
    compute_threads_.resize(threadnum);

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
void SegLineToHits(std::string temp, vector<string>& ret)
{
    ret.clear();
    size_t templen = temp.find(" ");
    while(templen!= string::npos)
    {
        if(templen!=0)
            ret.push_back(temp.substr(0,templen));
        temp=temp.substr(templen+1);
        templen = temp.find(" ");

    }
    if(temp.length()!=0)
        ret.push_back(temp);

}
int main()
{
    time_t t(0);
    srand((unsigned)time(&t));
    std::string cma_path= IZENECMA_KNOWLEDGE ;
    std::string knowledge_dir;//= MATCHER_KNOWLEDGE;
    string scd_file;//= TEST_SCD_PATH;
    string category_test_file;//
    string process_test_file;//
    string getSearchKeywords_test_file;//
    ifstream in;
    in.open("config",ios::in);
    string line;
    unsigned threadnum=4;
    unsigned testminute=0;
    while( getline(in, line))
    {
        if(line.find("knowledge_dir:")==0)
        {
            knowledge_dir=line.substr(line.find("knowledge_dir:")+string("knowledge_dir:").length());
        }
        else if(line.find("scd_path:")==0)
        {
            scd_file=line.substr(line.find("scd_path:")+string("scd_path:").length());
        }
        else if(line.find("category_test_file")==0)
        {
            category_test_file=line.substr(line.find("category_test_file:")+string("category_test_file:").length());
        }
        else if(line.find("process_test_file")==0)
        {
            process_test_file=line.substr(line.find("process_test_file:")+string("process_test_file:").length());
        }
        else if(line.find("getSearchKeywords_test_file")==0)
        {
            getSearchKeywords_test_file=line.substr(line.find("getSearchKeywords_test_file:")+string("getSearchKeywords_test_file:").length());
        }
        else if(line.find("threadnum")==0)
        {
            threadnum=boost::lexical_cast<int>(line.substr(line.find("threadnum:")+string("threadnum:").length()));
        }
        else if(line.find("testminute")==0)
        {
            testminute=boost::lexical_cast<int>(line.substr(line.find("testminute:")+string("testminute:").length()));
        }
    }
    cout<< cma_path<<"  "<<knowledge_dir<<" "<<scd_file<<" "<<category_test_file<<" "<<process_test_file<<"  "<<getSearchKeywords_test_file<<endl;
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
    if(!category_test_file.empty())
    {
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
            if(param.size()==10)
            {
                checkProcesss(matcher,param[0],param[1],param[2],param[3],param[4],param[5],param[6],param[7],param[8],param[9]);
                DocNum++;
            }
        }


    }
    cout<<DocNum<<endl;
    if(DocNum!=0)
    {
        cout<<"错误率："<<double(ProcessError)/DocNum<<endl;
    }
    sleep(10);
    in.close();
    ofstream out;

    if(!getSearchKeywords_test_file.empty())
    {
        int num =0;
        in.open(getSearchKeywords_test_file.c_str(),ios::in);
        string title;
        vector<string> hitanswer;
        while( getline(in, line))
        {
            num++;
            if(num%2==1)
            {
                title=line;
            }
            else
            {

                SegLineToHits(line,hitanswer);
                UString text(title, UString::UTF_8);
                std::list<std::pair<UString, double> > hits;
                std::list<UString> left;
                matcher.GetSearchKeywords(text, hits, left);
                //out<<title<<endl;
                vector<string> hitresult;
                for(std::list<std::pair<UString, double> >::iterator i=hits.begin(); i!=hits.end(); i++)
                {
                    //out<<toString(i->first)<<"  ";
                    hitresult.push_back(toString(i->first));
                }
                for(unsigned i=0; i<hitanswer.size(); i++)
                {
                    if(find(hitresult.begin(),hitresult.end(),hitanswer[i])==hitresult.end())
                        cout<<"error"<<title<<" "<<hitanswer[i]<<"hits but not found"<<endl;
                }
                out<<endl;
                for(std::list<UString>::iterator i=left.begin(); i!=left.end(); i++)
                {
                    // cout<<toString(*i)<<"  ";
                }
                //cout<<endl;

            }
        }
    }
    cout<<"错误率："<<double(ProcessError)/(40000+DocNum)<<endl;
    /*

    for(int i=0; i<10; i++)

        cout<<rand_str()<<endl;
    */

        MultiThreadProcessTest(scd_file,knowledge_dir,threadnum);
        MultiThreadGetCategoryTest(scd_file,knowledge_dir,testminute,threadnum);
        MultiThreadRandomProcessTest(scd_file,knowledge_dir,testminute,threadnum);
        MultiThreadSearchKeywordsTest(scd_file,knowledge_dir,testminute,threadnum);

    return 1;
};



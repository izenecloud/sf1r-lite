#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>

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






BOOST_AUTO_TEST_SUITE(ProductMatcher_test)


BOOST_AUTO_TEST_CASE(ProductMatcher_case)
{
        std::string cma_path= IZENECMA_KNOWLEDGE ;
        std::string knowledge_dir= MATCHER_KNOWLEDGE;
        //string scd_path;//="/home/lscm/6indexSCD/";
        /*
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
         */
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
        //if(!scd_path.empty())
        //{
            //LOG(ERROR)<<"scd path empty"<<std::endl;
            vector<std::pair<string,string> >  txt;
            txt.push_back(make_pair("限时包邮2012春装新款大摆半身裙波西米亚长裙夏季蓬蓬裙网纱裙","服饰鞋帽>女装>裙子"));
            txt.push_back(make_pair("秋冬新款欧美风厚实喇叭大裙摆黑灰裙裤打底裤显瘦裤裙","服饰鞋帽>女装>裙子"));
            txt.push_back(make_pair("韩国代购stylenanda左右不对称黑色长裙","服饰鞋帽>女装>裙子"));
            txt.push_back(make_pair("七匹狼运动专卖店,七匹狼羽绒服 可卸帽高档保暖白鸭绒外套男 冬装特价运动男装,七匹狼,羽绒服,可卸帽,高档,保暖,鸭绒,外套,冬装,特价,运动,男装 ","服饰鞋帽>男装>羽绒服"));
            txt.push_back(make_pair("IBM E420 E420(1141A86) Thinkpad 新品促销 ","电脑办公>电脑整机>笔记本"));
            txt.push_back(make_pair("大陆行货 Apple/苹果 IPHONE 4 (8G) iPhone4 全新正品 全国联保 ","手机数码>手机通讯>手机"));
            txt.push_back(make_pair("孝傲江湖 秋季新款女鞋高品质亮丽漆皮女时尚休闲女鞋蝴蝶结布鞋175222 黑色 35 ","服饰鞋帽>女鞋>布鞋"));

 
            for(unsigned i=0;i<txt.size();i++)
            {
               //cout<<txt[i]<<"  ";
               std::vector<UString> results;
               UString text(txt[i].first, UString::UTF_8);
               matcher.GetFrontendCategory(text, 3, results);
               //cout<<"result:"<<results.size();
               bool get=false;
               for(unsigned j=0;j<results.size();j++)
               {   string str;
                   results[j].convertString(str, UString::UTF_8);
                   //cout<<str;
                   if(str==txt[i].second)
                   {  
                       get=true;
                   }
               }
               BOOST_CHECK_EQUAL(get, true);
               //cout<<endl;
            }
            //matcher.Test(scd_path);
        
        /*
        {
        else
        {
            matcher.SetUsePriceSim(false);
            while(true)
            {
                std::string line;
                std::cerr<<"input text:"<<std::endl;
                getline(std::cin, line);
                boost::algorithm::trim(line);
                Document doc;
                doc.property("Title") = UString(line, UString::UTF_8);
                uint32_t limit = 3;
                std::vector<ProductMatcher::Product> products;
                matcher.Process(doc, limit, products);
                for(uint32_t i=0;i<products.size();i++)
                {
                    std::cout<<"[CATEGORY]"<<products[i].scategory<<std::endl;
                }
            }
        }
        */
}


BOOST_AUTO_TEST_SUITE_END()


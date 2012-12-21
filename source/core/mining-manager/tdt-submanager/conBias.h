#ifndef SF1R_CON_BIAS_HPP_
#define SF1R_CON_BIAS_HPP_
#include <log-manager/LogAnalysis.h>
#include <string.h>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include <icma/icma.h>
#include <log-manager/UserQuery.h>
using namespace std;
using namespace boost;
using namespace cma;


namespace sf1r
{

class ConBias
{
    string tokenize_dicpath_;
    Knowledge* knowledge_;
    Analyzer* analyzer_;
    CMA_Factory* factory_;
public:
ConBias(const string& cma_path):tokenize_dicpath_(cma_path)
{
    cout<<"conBiasBuild"<<endl;
    factory_ = CMA_Factory::instance();
    knowledge_ = factory_->createKnowledge();
    cout<<"cma_path"<<cma_path<<endl;
    knowledge_->loadModel( "utf8", cma_path.data() );
    analyzer_ = factory_->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING,0);
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE,77);
    analyzer_->setKnowledge(knowledge_);
    analyzer_->setPOSDelimiter("/");
};
~ConBias()
{
    delete knowledge_;
    delete analyzer_;
    
}
vector<pair<string,uint32_t> > getKeywordFreq(const std::string& content)
{
    //cout<<"getKeywordFreqBegin"<<endl;
    const char* result = analyzer_->runWithString(content.data());
    //cout<<result<<endl;
    string res(result);
    //log_<<res<<"  ";
    vector<string> ret;
    string temp=res;
    size_t templen = temp.find(" ");
    while(templen!= string::npos)
    {
        if(templen!=0)
        {
            if(templen>3)
            {
              ret.push_back(temp.substr(0,templen));
            }
            //SegWord(temp.substr(0,templen));
        }
        temp=temp.substr(templen+1);
        templen = temp.find(" ");

    }
    if(temp.length()>0)
    {
        ret.push_back(temp);
    }
    
    sort(ret.begin(),ret.end());
    //cout<<"ret.size"<<ret.size()<<endl;
    temp="";
    uint32_t tempNum=1;
    vector<pair<string,uint32_t> >    pairRet;
    for(unsigned i=0; i<ret.size(); i++)
    {
        if(ret[i]==temp)
        {
            tempNum++;
        }
        else
        {
           // cout<<temp<<"    "<<tempNum<<endl;
            pairRet.push_back(make_pair(temp,tempNum));
            tempNum=1;
            temp=ret[i];

        }
    }
   // cout<<"getKeywordFreqEnd"<<endl;
    return pairRet;
}

};

}
#endif

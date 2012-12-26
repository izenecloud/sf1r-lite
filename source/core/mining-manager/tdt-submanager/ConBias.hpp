#ifndef SF1R_CON_BIAS_HPP_
#define SF1R_CON_BIAS_HPP_

#include <icma/icma.h>

#include <iostream>
#include <vector>

using namespace cma;

namespace sf1r
{

class ConBias
{
    std::string tokenize_dicpath_;
    Knowledge* knowledge_;
    Analyzer* analyzer_;
    CMA_Factory* factory_;
public:
    ConBias(const std::string& cma_path):tokenize_dicpath_(cma_path)
    {
        std::cout<<"conBiasBuild"<<std::endl;
        factory_ = CMA_Factory::instance();
        knowledge_ = factory_->createKnowledge();
        std::cout<<"cma_path"<<cma_path<<std::endl;
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
    void getKeywordFreq(const std::string& content, std::vector<pair<std::string,uint32_t> >& pairRet)
    {
        //cout<<"getKeywordFreqBegin"<<endl;
        const char* result = analyzer_->runWithString(content.data());
        //cout<<result<<endl;
        std::string res(result);
        //log_<<res<<"  ";
        std::vector<std::string> ret;
        string temp=res;
        size_t templen = temp.find(" ");
        while(templen!= std::string::npos)
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

        std::sort(ret.begin(),ret.end());
        //cout<<"ret.size"<<ret.size()<<endl;
        temp="";
        uint32_t tempNum=1;
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
    }

};

}
#endif

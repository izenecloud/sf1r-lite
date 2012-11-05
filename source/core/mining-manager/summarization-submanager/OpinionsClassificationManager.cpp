#include "OpinionsClassificationManager.h"

using namespace std;
typedef izenelib::am::leveldb::Table<std::string, int> LevelDBType;

namespace sf1r
{
std::pair<int,int> getActualNumber(int k)
{
    return  std::make_pair(k/100000,k%100000);
}
int getStoreNumber(int a,int b)
{
    return  a*100000+b;
}

boost::tuple<int,int,int>  getActualNumberTuple(int k)
{
    return  boost::make_tuple(k/1000000,(k/1000)%1000,k%1000);
}
int getStoreNumberTuple(int a,int b,int c)
{
    return  a*1000000+b*1000+c;
}

std::string OpinionsClassificationManager::system_resource_path_;
OpinionsClassificationManager::OpinionsClassificationManager(const string& cma_path,const string& path)
    :modelPath_(cma_path)
    ,indexPath_(path)
{
    dictPath_=system_resource_path_+"/opinion";
    factory_ = CMA_Factory::instance();
    knowledge_ = factory_->createKnowledge();
    knowledge_->loadModel( "utf8", modelPath_.data() );
    analyzer_ = factory_->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING,0);
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE,77);
    analyzer_->setKnowledge(knowledge_);
    analyzer_->setPOSDelimiter("/");
    string logpath=path+"/OpinionsClassificationManager.log";
    log_.open(logpath.c_str(),ios::out);
    dbTable_=new LevelDBType(path+"/leveldb");
    log_<<" open"<<dbTable_->open()<<endl;
    loadAll(dictPath_);
    sort();
}

OpinionsClassificationManager::~OpinionsClassificationManager()
{
    saveAll(indexPath_);
    log_.close();
    dbTable_->close();
    delete knowledge_;
    delete analyzer_;
    delete dbTable_;
}

void OpinionsClassificationManager::loadAll(string path)
{

    log_<<"start!"<<endl;
    load(goodWord_,path+"/goodWord");
    load(badWord_,path+"/badWord");
    load(reverseWord_,path+"/reverseWord");
    load(goodPair_,path+"/goodPair");
    load(badPair_,path+"/badPair");
}

void OpinionsClassificationManager::load(vector<string>& vec,string pathname)
{

    ifstream in;
    in.open(pathname.c_str(), ios::in);
    if(in.is_open())
    {
        while(!in.eof())
        {
            std::string temp;
            getline(in, temp);
            vec.push_back(temp);
        }
        in.close();
    }
    else
    {
        LOG(ERROR)<<"Load "<<pathname<<" error!"<<endl;
    }
}

void OpinionsClassificationManager::load(vector<pair<string,string> >& vec,string pathname)
{
    ifstream in;
    in.open(pathname.c_str(), ios::in);
    if(in.is_open())
    {
        while(!in.eof())
        {
            std::string temp;
            getline(in, temp);
            size_t templen = temp.find(" ");
            if(templen== string::npos)
            {
                break;
            }
            vec.push_back(make_pair(temp.substr(0,templen),temp.substr(templen+1)));
        }
        in.close();
    }
    else
    {
        LOG(ERROR)<<"Load "<<pathname<<" error!"<<endl;
    }
}

void  OpinionsClassificationManager::save(vector<string>& vec,string pathname)
{
    ofstream out;
    out.open(pathname.c_str(), ios::out);
    for(unsigned  i=0; i<vec.size(); i++)
    {
        out<<vec[i]<<endl;
    }
    out.close();
}

void  OpinionsClassificationManager::save(vector<pair<string,string> >& vec,string pathname)
{
    ofstream out;
    out.open(pathname.c_str(), ios::out);
    for(unsigned  i=0; i<vec.size(); i++)
    {
        out<<vec[i].first<<"  "<<vec[i].second<<endl;
    }
    out.close();
}

void  OpinionsClassificationManager::saveSelect(vector<string>& vec,string pathname)
{
    ofstream out;
    out.open(pathname.c_str(), ios::out);
    int goodnum,badnum,storenum,uncommentnum;
    for(unsigned  i=0; i<vec.size(); i++)
    {

        if(dbTable_->get(vec[i], storenum))
        {
            boost::tuple<int,int,int> p=getActualNumberTuple(storenum);
            goodnum=p.get<0>();
            badnum=p.get<1>();
            uncommentnum=p.get<2>();
            if(goodnum+badnum+uncommentnum>20)
            {

                if(goodnum<2*badnum)
                {
                    out<<vec[i]<<endl;
                    //out<<goodnum<<"  "<<badnum<<" "<<uncommentnum<<endl;
                }
            }
        }
        //out<<endl;

    }
    out.close();
}

void  OpinionsClassificationManager::saveAll(string path)
{
    sort();
    save( goodWordUseful_,path+"/goodWordUseful");
    save( badWordUseful_,path+"/badWordUseful");
    saveSelect( wordSelfLearn_,path+"/WordSelfLearn");

}

vector<string> OpinionsClassificationManager::SegQuery(const std::string& query)
{

    const char* result = analyzer_->runWithString(query.data());
    string res(result);
    //log_<<res<<"  ";
    string temp=res;
    size_t templen = temp.find(" ");
    vector<string> ret;
    while(templen!= string::npos)
    {
        if(templen!=0)
        {
            ret.push_back(temp.substr(0,templen));
            //SegWord(temp.substr(0,templen));
        }
        temp=temp.substr(templen+1);
        templen = temp.find(" ");

    }
    if(temp.length()>0)
    {
        ret.push_back(temp);
    }
    return ret;

}

vector<string> OpinionsClassificationManager::SegWord(const std::string& Word)
{
    izenelib::util::UString UWord(Word, izenelib::util::UString::UTF_8);
    size_t len = UWord.length();

    vector<string> result;
    int k=0;
    for(unsigned  i = 0; i < len; i++)
    {
        string str;
        if(!UWord.isChineseChar(i))
        {

            if(UWord.isChineseChar(i-1))
            {
                k=i;
            }


        }
        else
        {
            if(!UWord.isChineseChar(i-1))
            {
                UWord.substr(k,i-k).convertString(str, izenelib::util::UString::UTF_8);
                vector<string> temp=SegQuery(str);
                result.insert(result.end(),temp.begin(),temp.end());
            }

            UWord.substr(i,1).convertString(str, izenelib::util::UString::UTF_8);

        }

        result.push_back(str);
    }
    return result;
    //分词//TODO
}

void OpinionsClassificationManager::getWordFromTrainData()
{
    for(unsigned  i=0; i<trainData_.size(); i++)
    {
        vector<string> wordvec=SegQuery(trainData_[i].first);
        bool good=trainData_[i].second;
        reverseDeal(wordvec,good);
        for(unsigned  j=0; j<wordvec.size(); j++)
        {
            insertWord(wordvec[j],good);
        }
    }
}

bool OpinionsClassificationManager::isreverse(string word)
{
    vector<string>::iterator it=find(reverseWord_.begin(),reverseWord_.end(),word); //查找3
    if ( it == reverseWord_.end() )
    {
        return false;
    }
    else
    {
        return true;
    }
}

void OpinionsClassificationManager::reverseDeal(vector<string>& wordvec,bool& good)
{
    for(vector<string>::iterator it=wordvec.begin(); it!=wordvec.end(); it++)
    {
        if(isreverse((*it)))
        {
            good=!good;                                //不得不
            it=wordvec.erase(it);
        }
        if(it==wordvec.end())
        {
            break;
        }

    }
}

void OpinionsClassificationManager::insertWord(string word,bool good)
{
    int storenum;
    int goodnum;
    int badnum;
    if(dbTable_->get(word, storenum))
    {
        std::pair<int,int> p=getActualNumber(storenum);
        goodnum=p.first;
        badnum=p.second;
        if(good)
        {
            goodnum++;
        }
        else
        {
            badnum++;
        }
        storenum=getStoreNumber(goodnum,badnum);
        dbTable_->del(word);
    }
    else
    {
        if(good)
        {
            goodnum=1;
            badnum=0;
        }
        else
        {
            goodnum=0;
            badnum=1;
        }
        storenum=getStoreNumber(goodnum,badnum);
        wordList_.push_back(word);
    }
    dbTable_->insert(word, storenum);
}

void  OpinionsClassificationManager::ClassfyWord(string word)
{
    int storenum;
    int goodnum;
    int badnum;
    if(dbTable_->get(word, storenum))
    {
        std::pair<int,int> p=getActualNumber(storenum);
        goodnum=p.first;
        badnum=p.second;
        if(  (goodnum+badnum)>cutoff_)
        {
            double possibility=double(goodnum)/(goodnum+badnum);
            if(possibility>accu_)
            {
                goodWord_.push_back(word);

            }
            else
            {
                if( (1-possibility)>accu_)
                {
                    badWord_.push_back(word);
                }
                else
                {
                    pairWord_.push_back(word);
                }
            }
        }
    }
};

void  OpinionsClassificationManager::ClassfyWordVector()
{
    for(list<string>::iterator it=wordList_.begin(); it!=wordList_.end(); it++)
    {
        ClassfyWord((*it));
    }
}

void OpinionsClassificationManager::getPairFromTrainData()
{
    for(unsigned  i=0; i<trainData_.size(); i++)
    {
        vector<string> wordvec=SegQuery(trainData_[i].first);
        bool good=trainData_[i].second;
        reverseDeal(wordvec,good);
        for(unsigned  j=0; j<wordvec.size()-1; j++)
        {
            if(find(pairWord_.begin(),pairWord_.end(),wordvec[j])!=pairWord_.end())
            {

                for(unsigned  k=j+1; k<wordvec.size(); k++)
                {
                    if(find(pairWord_.begin(),pairWord_.end(),wordvec[k])!=pairWord_.end())
                    {
                        insertPair(wordvec[j],wordvec[k],good);
                    }
                }
            }
        }
    }
}

void OpinionsClassificationManager::insertPair(const string& w1,const string& w2,bool good)
{
    int storenum;
    int goodnum;
    int badnum;
    string word;
    bool ok;
    string word1 = w1, word2 = w2;
    if(dbTable_->get(word1+word2, storenum))
    {
        word=word1+word2;
    }
    else
    {
        if(dbTable_->get(word2+word1, storenum))
        {
            word=word1;
            word1=word2;
            word2=word;
            word=word1+word2;
        }
        else
        {
            ok=false;
        }
    }
    if(ok)
    {
        std::pair<int,int> p=getActualNumber(storenum);
        goodnum=p.first;
        badnum=p.second;
        if(good)
        {
            goodnum++;
        }
        else
        {
            badnum++;
        }
        storenum=getStoreNumber(goodnum,badnum);
    }
    else
    {
        if(good)
        {
            goodnum=1;
            badnum=0;
        }
        else
        {
            goodnum=0;
            badnum=1;
        }
        storenum=getStoreNumber(goodnum,badnum);
        pairList_.push_back(std::make_pair(word1,word2));
    }
    dbTable_->insert(word, storenum);
}

double  OpinionsClassificationManager::PMI(const string& word1,const string& word2)
{
    int storenum=0,storenum1=0,storenum2=0;
    int goodnum=0,goodnum1=0,goodnum2=0;
    int badnum=0,badnum1=0,badnum2=0;
    string word=word1+word2;
    double pmigood,pmibad;
    if(dbTable_->get(word1, storenum1))
    {
        std::pair<int,int> p=getActualNumber(storenum1);
        goodnum1=p.first;
        badnum1=p.second;
    }
    if(dbTable_->get(word2, storenum2))
    {
        std::pair<int,int> p=getActualNumber(storenum2);
        goodnum2=p.first;
        badnum2=p.second;
    }
    if(dbTable_->get(word, storenum))
    {
        std::pair<int,int> p=getActualNumber(storenum);
        goodnum=p.first;
        badnum=p.second;
    }
    if(goodnum+badnum<cutoff_)
    {
        return 0.5;
    }
    else
    {
        pmigood=double(goodnum)/(goodnum1*goodnum2);
        pmibad=double(badnum)/(badnum1*badnum2);
        return  pmigood/(pmigood+pmibad);
    }
    /**/

}
void  OpinionsClassificationManager::ClassifyPair(const pair<string,string>& p)
{
    double possibility=PMI(p.first,p.second);
    if(possibility>accu_)
    {
        goodPair_.push_back(p);

    }
    else
    {
        if( (1-possibility)>accu_)
        {
            badPair_.push_back(p);
        }
    }
}

void  OpinionsClassificationManager::ClassifyPairVector(const vector<pair<string,string> >& p)
{
    for(list<pair<string,string> >::iterator it=pairList_.begin(); it!=pairList_.end(); it++)
    {
        ClassifyPair((*it));
    }
}

void  OpinionsClassificationManager::sort()
{
    std::sort(goodWord_.begin(),goodWord_.end());
    std::sort(badWord_.begin(),badWord_.end());
    std::sort(reverseWord_.begin(),reverseWord_.end());
}

bool  OpinionsClassificationManager::include(const string& word, const vector<string>& strvec)
{
    /*
       vector<string>::iterator it=find(strvec.begin(),strvec.end(),word);
       if ( it == strvec.end() )
       {
       return false;
       }
       else
       {
       return true;
       }
       */
    int front=0;
    int end=strvec.size()-1;
    int mid=(front+end)/2;
    while(front<end && strvec[mid]!=word)
    {
        if(strvec[mid]<word)front=mid+1;
        if(strvec[mid]>word)end=mid-1;
        mid=front + (end - front)/2;
    }
    if(strvec[mid]!=word)
    {
        return false;
    }
    else
    {
        return true;
    }

}

bool  OpinionsClassificationManager::include(const pair<string,string>& wordpair, const vector<pair<string,string> >& pairvec)
{
    vector<pair<string,string> >::const_iterator it=find(pairvec.begin(),pairvec.end(),wordpair);
    if ( it == pairvec.end() )
    {
        return false;
    }
    else
    {
        return true;
    }
}

void OpinionsClassificationManager::insert(string word,int cat)
{
    int storenum=0;
    int goodnum=0;
    int uncommentnum=0;
    int badnum=0;
    if(dbTable_->get(word, storenum))
    {
        boost::tuple<int,int,int> p=getActualNumberTuple(storenum);
        goodnum=p.get<0>();
        badnum=p.get<1>();
        uncommentnum=p.get<2>();
        if(cat==0)
        {
            goodnum++;
        }
        if(cat==1)
        {
            badnum++;
        }
        if(cat==2)
        {
            uncommentnum++;
        }
        storenum=getStoreNumberTuple(goodnum,badnum,uncommentnum);
        dbTable_->del(word);
    }
    else
    {
        if(cat==0)
        {
            goodnum++;
        }
        if(cat==1)
        {
            badnum++;
        }
        if(cat==2)
        {
            uncommentnum++;
        }
        storenum=getStoreNumberTuple(goodnum,badnum,uncommentnum);
        wordSelfLearn_.push_back(word);
    }
    dbTable_->insert(word, storenum);
}
int OpinionsClassificationManager::dealwithWordPair(string wordpair,bool& reverse)
{
    vector<string> charvec=SegWord(wordpair);
    string tempword="";
    int k=0;
    int score=0;
    //log_<<"wordpair"<<wordpair<<endl;
    for(unsigned i=charvec.size(); i>1; i--)
    {
        tempword="";
        for(unsigned j=0; j<i; j++)
        {
            tempword+=charvec[j];

        }
        if(tempword.length()==0)
        {
            break;
        }
        if(include(tempword,reverseWord_))
        {
            reverse=!reverse;
            log_<<tempword<<"reverse";
            k=i;
            break;
        }
        else
        {
            if(include(tempword,goodWord_))
            {
                score++;
                log_<<tempword<<"+1";
                k=i;
                break;
            }
            else
            {
                if(include(tempword,badWord_))
                {
                    score--;
                    log_<<tempword<<"-1";
                    k=i;
                    break;
                }
            }
        }

    }
    k++;
    tempword="";

    for(unsigned i=k; i<charvec.size(); i++)
    {
        tempword+=charvec[k];
    }
    if(tempword.length()<=1)
    {
        return score;
    }
    else
    {
        score+=dealwithWordPair(tempword,reverse);
        return score;
    }
}

int OpinionsClassificationManager::getResult(const string& Sentence)
{
    vector<string> wordvec=SegQuery(Sentence);
    int reverse=1;
    //reverseDeal(wordvec,reverse);
    int score=0;
    vector<string> temp;
    log_<<reverse<<"  ";
    int reversenum=0;
    for(unsigned j=0; j<wordvec.size(); j++)
    {
        string wordpair;
        if(j<wordvec.size()-1)
        {
            wordpair=wordvec[j]+wordvec[j+1];
        }
        else
        {
            wordpair=wordvec[j];
        }
        if(include(wordpair,reverseWord_))
        {
            log_<<wordpair<<"(-1)*(";
            reverse=-reverse;
            j++;
            reversenum++;

        }
        else
        {
            if(include(wordpair,goodWord_))
            {
                log_<<wordpair<<"+1";
                score+=reverse;
                j++;
                //goodWordUseful_.push_back(wordpair);
            }

            else
            {
                if(include(wordpair,badWord_))
                {
                    log_<<wordpair<<"-1";
                    score-=reverse;
                    j++;
                   // badWordUseful_.push_back(wordpair);
                }
                else
                {
                    if(include(wordvec[j],reverseWord_))
                    {
                        log_<<wordvec[j]<<"(-1)*(";
                        reverse=-reverse;

                        reversenum++;

                    }
                    else
                    {
                        if(include(wordvec[j],goodWord_))
                        {
                            log_<<wordvec[j]<<"+1";
                            score+=reverse;

                           // goodWordUseful_.push_back(wordvec[j]);
                        }
                        else
                        {

                            if(include(wordvec[j],badWord_))
                            {
                                log_<<wordvec[j]<<"-1";
                                score-=reverse;

                              //  badWordUseful_.push_back(wordvec[j]);
                            }
                            else
                            {
                                log_<<wordvec[j]<<"0";
                                temp.push_back(wordvec[j]);
                            }
                        }
                    }


                }

                // score+=dealwithWordPair(wordvec[j],reverse);
            }
        }
    }
    for(int j=0; j<reversenum; j++)
    {
        log_<<")";
    }
    if((score>0))
    {
        log_<<"优点"<<endl;
    }
    else
    {
        if(score==0)
        {
            log_<<"无法评价！"<<endl;
        }
        else
        {
            log_<<"缺点"<<endl;
        }
    }
    log_<<"未识别"<<temp.size()<<endl;
    /*
       if(score>0)
       {
       for(int i=0;i<temp.size();i++)
       {

       insert(temp[i],0);
       }
       }
       if(score<0)
       {
       for(int i=0;i<temp.size();i++)
       {

       insert(temp[i],1);
       }
       }

       if(score==0)
       {
       for(int i=0;i<temp.size();i++)
       {

       insert(temp[i],2);
       }
       }
       */
    return score;
}

void SegmentToSentece(const string& Segment, vector<string>& Sentence)
{
    string temp = Segment;
    string dot=",";
    size_t templen = 0;
    boost::algorithm::replace_all(temp,"！",",");
    boost::algorithm::replace_all(temp,"。",",");
    boost::algorithm::replace_all(temp,"～",",");
    boost::algorithm::replace_all(temp,"?",",");
    boost::algorithm::replace_all(temp,"？",",");
    boost::algorithm::replace_all(temp,".",",");
    boost::algorithm::replace_all(temp,"!",",");
    boost::algorithm::replace_all(temp,"~",",");
    boost::algorithm::replace_all(temp,":",",");
    boost::algorithm::replace_all(temp,"：",",");
    boost::algorithm::replace_all(temp,",",",");
    boost::algorithm::replace_all(temp," ",",");
    boost::algorithm::replace_all(temp,";",",");
    boost::algorithm::replace_all(temp,"；",",");
    boost::algorithm::replace_all(temp,"，",",");

    for(unsigned i=0; i<temp.length(); i++)
    {
        if(temp[i]=='\n')
        {
            temp[i]=',';
        }
    }
    /**/
    while(!temp.empty())
    {
        size_t len1 = temp.find(",");
        size_t len2 = temp.find("。");
        if(len1 != string::npos || len2 != string::npos)
        {
            if(len1 == string::npos)
            {
                templen = len2;
            }
            else if(len2 == string::npos)
            {
                templen = len1;
            }
            else
            {
                templen = min(len1,len2);
            }
            Sentence.push_back(temp.substr(0,templen) );
            temp = temp.substr(templen + dot.length());
        }
        else
        {
            Sentence.push_back(temp);
            break;
        }
    }
}

std::pair<UString,UString> OpinionsClassificationManager::test(const string& Segment)
{
    log_<<"test"<<endl;
    //cout<<Segment<<endl;
    log_<<Segment<<endl;
    string advantage="";
    string disadvantage="";

    vector<string> Sentence;
    SegmentToSentece(Segment, Sentence);
    log_<<"Sentence"<<Sentence.size()<<endl;
    if(Segment.length()<200)
    {
        for(unsigned i=0; i<Sentence.size(); i++)
        {
            log_<<Sentence[i]<<"  ";
            int score=getResult(Sentence[i]);
            if(score>0)
            {
                advantage=advantage+Sentence[i]+",";
            }
            else
            {
                if(score<0)
                {
                    disadvantage=disadvantage+Sentence[i]+",";
                }
            }
            // log_<<getResult(Sentence[i])<<" "<<endl;
        }
    }
    UString Uadvantage(advantage, izenelib::util::UString::UTF_8);
    UString Udisadvantag(disadvantage, izenelib::util::UString::UTF_8);
    /*  */
    return std::make_pair(Uadvantage,Udisadvantag);
}

}


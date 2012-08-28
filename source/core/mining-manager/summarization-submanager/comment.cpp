#include "comment.h"
#include <glog/logging.h>
#include <math.h>   

#define VERY_LOW  -50

using namespace std;

typedef std::pair<std::string,int> seedPairT;
typedef std::pair<std::string, double> seedPair2T;

struct seedPairCmp {
  bool operator() (const seedPairT seedPair1,const seedPairT seedPair2) 
        { 
             return seedPair1.second > seedPair2.second;
        }

} seedPairCmp;

struct seedPairCmp2 {
  bool operator() (const seedPair2T seedPair1,const seedPair2T seedPair2) 
        { 
             return seedPair1.second > seedPair2.second;
        }

} seedPairCmp2;


namespace sf1r
{
    inline bool IsNeedIgnoreChar(UCS2Char c)
    {
        return !izenelib::util::UString::isThisChineseChar(c);
    }

OpinionsManager::OpinionsManager(const string& colPath)
{
    /*
    factory = CMA_Factory::instance();
    knowledge = factory->createKnowledge();
    knowledge->loadModel( enc.data(), cate.data() );
    analyzer = factory->createAnalyzer();
    analyzer->setOption(Analyzer::OPTION_TYPE_POS_TAGGING,0);
    analyzer->setOption(Analyzer::OPTION_ANALYSIS_TYPE,77);
    analyzer->setKnowledge(knowledge);
    analyzer->setPOSDelimiter(posDelimiter.data());
    */
    Ngram_= new Ngram(colPath);
    string logpath=colPath+"/OpinionsManager.log";
    out.open(logpath.c_str(), ios::out);
    windowsize = 4;
    encodingType_ = UString::UTF_8;
}

OpinionsManager::~OpinionsManager()
{
    out.close();
    //delete factory;
    //delete knowledge;
    //delete analyzer;
    delete Ngram_;
}

void OpinionsManager::setWindowsize(int C)
{
    windowsize=C;
}

void OpinionsManager::setEncoding(izenelib::util::UString::EncodingType encoding)
{
    encodingType_ = encoding;
}

void OpinionsManager::setComment(const SentenceContainerT& in_sentences)
{
    Z.clear();
    out << "----- original comment (only Chinese) start-----" << endl;
    for(size_t i = 0; i < in_sentences.size(); i++)
    { 
        izenelib::util::UString ustr(in_sentences[i], encodingType_);
        izenelib::util::UString::iterator uend = std::remove_if(ustr.begin(), ustr.end(), IsNotChineseChar);
        if(uend == ustr.begin())
            continue;
        ustr.assign(ustr.begin(), uend);
        Z.push_back("");
        ustr.convertString(Z[Z.size() - 1], encodingType_);
        out << Z[Z.size() - 1] << endl;
    }
    out << "----- original comment end-----" << endl;
    out.flush();
}

void OpinionsManager::setSigma(double SigmaRep_,double SigmaRead_,double SigmaSim_,double SigmaLength_)
{
    SigmaRep=SigmaRep_;
    SigmaRead=SigmaRead_;
    SigmaSim=SigmaSim_;
    SigmaLength=SigmaLength_;
    out<<"SigmaRep:"<<SigmaRep<<endl;
    out<<"SigmaRead:"<<SigmaRead<<endl;
    out<<"SigmaSim:"<<SigmaSim<<endl;
    out<<"SigmaLength:"<<SigmaLength<<endl;
}

void OpinionsManager::beginSeg(const string& inSentence, string& outSentence)
{
    char* charOut = NULL;
    analyzer->runWithStream(inSentence.c_str(),charOut);
    string outString(charOut);
    outSentence=outString;

}

void OpinionsManager::stringToWordVector(const std::string& Mi, SentenceContainerT& words)
{
    izenelib::util::UString ustr(Mi, encodingType_);
    size_t len = ustr.length();
    words.reserve(words.size() + len);
    for(size_t i = 0; i < len; i++)
    {  
        string strseg;
        ustr.substr(i,1).convertString(strseg, encodingType_);
        words.push_back(strseg);
    }
    //分词//TODO
}

void OpinionsManager::WordVectorToString(std::string& Mi,const WordSegContainerT& words)
{
    Mi = "";
    for(size_t i = 0; i < words.size(); i++)
    {
        Mi = Mi + words[i];
    }
}

//score
//double OpinionsManager::ScoreRep(const WordSegContainerT& M)
//{
//    double Srep=0;
//    for(size_t i=0; i<M.size(); i++)
//    {
//        Srep+=ScoreRep(M[i]);
//    }
//    return Srep;
//}
//
//double OpinionsManager::ScoreRep(const std::string& Mi)
//{
//    WordSegContainerT words;
//    stringToWordVector(Mi,words);
//    return Srep(words);
//}

double OpinionsManager::Srep(const WordSegContainerT& words)
{
    if(words.empty())
        return (double)VERY_LOW;


    std::string phrase;
    WordVectorToString(phrase, words);
    CachedStorageT::iterator it = cached_srep.find(phrase);
    if(it != cached_srep.end())
    {
        return it->second;
    }

    size_t n = words.size();
    double sigmaPMI = 0;
    for(size_t i = 0; i < n; i++)
    {
        sigmaPMI += PMIlocal(words, i, windowsize);
        // if(windowsize==6)
        // {out<<"PMI:"<<PMIlocal(words,i,windowsize)<<endl;}
    }
    sigmaPMI /= double(n);

    if(words.size() == 2 && sigmaPMI < SigmaRep)
        sigmaPMI = SigmaRep + 0.01;

    cached_srep[phrase] = sigmaPMI;

    return sigmaPMI;
}

//double OpinionsManager::ScoreRead(const NgramPhraseT& M)
//{
//    double Sread=0;
//    for(size_t i=0; i<M.size(); i++)
//    {
//        Sread+=ScoreRead(M[i]);
//    }
//    return Sread;
//}
//
//double OpinionsManager::ScoreRead(const std::string& Mi)
//{
//    SentenceContainerT words;
//    stringToWordVector(Mi,words);
//    return Sread(words);
//}

double OpinionsManager::Sread(const NgramPhraseT& words)
{
    std::string phrase;
    WordVectorToString(phrase, words);
    CachedStorageT::iterator it = cached_sread.find(phrase);
    if(it != cached_sread.end())
    {
        return it->second;
    }
    double possiblity=1;
    int K=0;
    for(size_t i = 1; i < words.size(); i++)
    {
        if(i==1)
        {
            possiblity *= Ngram_->Possiblity(words[1], words[0]);
            K += 1;
        }
        else
        {
            possiblity *= Ngram_->Possiblity(words[i], words[i-1], words[i-2]);
            K += 1;
        }
        /*
           else
           {
           possiblity *= Ngram_->Possiblity(words[i], words[i-1], words[i-2], words[i-3]);

           K += 1;
           }
           */

    }
    double score = log(possiblity) / log(double(2)) / K;
    assert(score < 0);
    if(score < (double)VERY_LOW*(double)words.size())
        score = (double)VERY_LOW*(double)words.size();
    //if(score > VERY_LOW*words.size())
    //    LOG(INFO) << "sread is greate than VERY_LOW. " << phrase << "," << score << endl;
    cached_sread[phrase] = score;
    return score;
}

double OpinionsManager::Score(const NgramPhraseT& words)
{
    return /*Sread(words) +*/ Srep(words) /*+ 0.25*words.size()*/;
}

double OpinionsManager::Sim(const std::string& Mi, const std::string& Mj)
{
    WordSegContainerT wordsi, wordsj;
    stringToWordVector(Mi, wordsi);
    stringToWordVector(Mi, wordsj);
    return Sim(wordsi, wordsj);
}

double OpinionsManager::Sim(const NgramPhraseT& wordsi, const NgramPhraseT& wordsj)
{
    //out<<endl;
    size_t sizei = wordsi.size();
    size_t sizej = wordsj.size();
    size_t same = 0;
    if( sizei <= 2 && sizej <= 2 )
    {

        for(size_t i = 0; i < sizei && i < sizej; i++)
            if(wordsi[i] == wordsj[i])
            {
                same++;
            }
        return double(same)/double(sizei + sizej - same);
    }
    std::string wordsi_phrase;
    std::string wordsj_phrase;
    WordVectorToString(wordsi_phrase, wordsi);
    WordVectorToString(wordsj_phrase, wordsj);
    if(wordsi_phrase.find(wordsj_phrase) != string::npos ||
        wordsj_phrase.find(wordsi_phrase) != string::npos)
    {
        return 0.9;
    }
    boost::unordered_map< std::string, int >  words_hash;
    for(size_t i = 0; i < sizei; ++i)
    {
        words_hash[wordsi[i]] = 1;
    }

    for(size_t j = 0; j < sizej; j++)
    {
        if(words_hash.find(wordsj[j]) != words_hash.end())
        {
            if(words_hash[wordsj[j]] == 1)
            {
                words_hash[wordsj[j]] += 1;
                same++;
            }
        }
    }
    return double(same)/double(sizei + sizej - same);//Jaccard similarity
}

//double OpinionsManager::getMScore(const WordSegContainerT& M)
//{
//    return ScoreRep(M)+ScoreRead(M);
//}

//subject
//bool OpinionsManager::SubjectLength(const SentenceContainerT& M)
//{
//    int Length=0;
//    for(size_t i=0; i<M.size(); i++)
//    {
//        Length+=M[i].length();
//    }
//    return Length<SigmaLength;
//}
//
//bool OpinionsManager::SubjectRead(const std::string& Mi)
//{
//    return ScoreRead(Mi)>SigmaRead;
//}
//bool OpinionsManager::SubjectRead(const SentenceContainerT& M)
//{
//    for(size_t i=0; i<M.size(); i++)
//    {
//        if(!SubjectRead(M[i]))
//        {
//            return false;
//        }
//    }
//    return true;
//}
//bool OpinionsManager::SubjectRep(const std::string& Mi)
//{
//    return ScoreRep(Mi)>SigmaRep;
//}
//bool OpinionsManager::SubjectRep(const SentenceContainerT& M)
//{
//    for(size_t i=0; i<M.size(); i++)
//    {
//        if(!SubjectRep(M[i]))
//        {
//            return false;
//        }
//    }
//    return true;
//}
//bool OpinionsManager::SubjectSim(const SentenceContainerT& M)
//{
//    for(size_t i=0; i<M.size()-1; i++)
//    {
//        for(size_t j=i+1; j<M.size(); j++)
//        {
//            if(Sim(M[i],M[j])<SigmaSim)
//            {
//                return false;
//            }
//        }
//    }
//    return true;
//}
//bool OpinionsManager::Subject(const SentenceContainerT& M)
//{
//    return SubjectLength(M)&&SubjectRead(M)&&SubjectRep(M)&&SubjectSim(M);
//}

//rep
double OpinionsManager::PMIlocal(const WordSegContainerT& words,
    const int& offset, int C) //C=window size
{
    double Spmi = 0;
    int size = words.size();
    for(int i = max(0, offset - C); i < min(size, offset + C); i++)
    {
        if(i != offset)
        {
            Spmi += PMImodified(words[offset], words[i], C);
        }
    }
    return Spmi/double(2*C);
}

double OpinionsManager::PMImodified(const std::string& Wi, const std::string& Wj, int C)
{
    WordJoinPossibilityMapT::iterator join_it = cached_pmimodified_.find(Wi);
    if( join_it != cached_pmimodified_.end() )
    {
        WordPossibilityMapT::iterator pit = join_it->second.find(Wj);
        if(pit != join_it->second.end())
        {
            return pit->second;
        }
    }
    int s = Z.size();
    double ret  = log( ( Possib(Wi,Wj) * CoOccurring(Wi,Wj,C) ) * 
        max(double(s), 10 + Z.size()*0.5) / ( Possib(Wi)*Possib(Wj) ) ) / log(2);
    if(ret < (double)VERY_LOW)
    {
        cached_pmimodified_[Wi][Wj] = (double)VERY_LOW;
        return (double)VERY_LOW;
    }
    //LOG(INFO) << "PMImodified greate than VERY_LOW: " << Wi << "," << Wj << "," << ret;
    cached_pmimodified_[Wi][Wj] = ret;
    return ret;
}

double OpinionsManager::CoOccurring(const std::string& Wi, const std::string& Wj, int C)
{
    int Poss=0;
    for(size_t j = 0; j < Z.size(); j++)
    {
        if(CoOccurringInOneSentence(Wi, Wj, C, Z[j]))
           Poss++; 
    }

    return Poss;
}

bool OpinionsManager::CoOccurringInOneSentence(const std::string& Wi,
    const std::string& Wj, int C, const std::string& sentence)
{
    UString ustr(sentence, encodingType_);
    UString uwi(Wi, encodingType_);
    UString uwj(Wj, encodingType_);

    size_t loci = ustr.find(uwi);
    while(loci != UString::npos)
    {
        size_t locj = ustr.find(uwj, max((int)loci - C, 0));
        if(locj != UString::npos && abs((int)locj - (int)loci) <= int(C + uwi.size() + uwj.size()))
            return true;
        loci = ustr.find(uwi, loci + 1);
    }
    return false;
}

double OpinionsManager::Possib(const std::string& Wi, const std::string& Wj)
{
    int Poss=0;
    for(size_t j = 0; j < Z.size(); j++)
    {
        size_t loci = Z[j].find(Wi);
        size_t locj = Z[j].find(Wj);
        if( locj != string::npos && loci != string::npos )
        {
            Poss++;
        }
    }
    return Poss;
}

double OpinionsManager::Possib(const std::string& Wi)
{
    int Poss=0;
    if(word_freq_insentence_.find(Wi) != word_freq_insentence_.end())
    {
        return word_freq_insentence_[Wi];
    }
    for(size_t j = 0; j < Z.size(); j++)
    {
        size_t loc = Z[j].find(Wi);
        if( loc != string::npos )
        {
            Poss++;
        }
    }
    word_freq_insentence_[Wi] = Poss;
    return Poss;
}

bool OpinionsManager::GenCandidateWord(WordSegContainerT& top_wordlist)
{
    WordSegContainerT orig_wordlist;
    for(size_t j = 0; j < Z.size(); j++)
    {
        stringToWordVector(Z[j], orig_wordlist);
    }

    LOG(INFO) << "total words: " << orig_wordlist.size() << std::endl;

    string wordTemp="";
    WordSegContainerT::iterator it = orig_wordlist.begin();
    WordSegContainerT::iterator it_end = orig_wordlist.end();

    // count frequency.
    for(; it != it_end; ++it)
    {
        wordTemp = (*it);
        if(!wordTemp.empty() && wordTemp != "，" 
            && wordTemp != "。" && wordTemp != "！" )
        {
            if(word_freq_records_.find(wordTemp) == word_freq_records_.end())
            {
                word_freq_records_[wordTemp] = 1;
            }
            else
            {
                word_freq_records_[wordTemp] += 1;
            }
        }
    }

    LOG(INFO) << "total different words: " << word_freq_records_.size() << std::endl;
    WordPriorityQueue_  topk_words;
    topk_words.Init(min((size_t)400, word_freq_records_.size()));
    bool need_add_single = false;
    if(word_freq_records_.size() < 400)
    {
        need_add_single = true;
    }
    WordFreqMapT::const_iterator cit = word_freq_records_.begin();
    while(cit != word_freq_records_.end())
    {
        if(cit->second > 1 || need_add_single)
        {
            topk_words.insert(std::make_pair(cit->first, cit->second));
        }
        ++cit;
    }

    size_t count = topk_words.size();
    top_wordlist.resize(count);
    LOG(INFO) << "top k words: " << count << std::endl;

    for (size_t i = 0; i < count; ++i)
    {
        const WordFreqPairT& item = topk_words.pop();
        //LOG(INFO) << "top " << i << " , " << item.first << ", cnt:" << item.second << std::endl;
        top_wordlist[count - i - 1] = item.first;
    }

    return true;
}

bool OpinionsManager::GenSeedBigramList(const WordSegContainerT& CandidateWord,
    BigramPhraseContainerT& resultList)
{
    if (CandidateWord.empty())
    {return true;}

    std::vector<string> bigram_words;
    bigram_words.resize(2);
    for(size_t i = 0; i < CandidateWord.size(); i++)
    {
        for(size_t j = 0; j < CandidateWord.size(); j++)
        {
            if(i != j)
            {
                bigram_words[0] = CandidateWord[i];
                bigram_words[1] = CandidateWord[j];
                if( (Sread(bigram_words) >= SigmaRead) && 
                    (Srep(bigram_words) >= SigmaRep) )
                {
                    resultList.push_back(std::make_pair(CandidateWord[i],CandidateWord[j]));
                    //LOG(INFO) << "add seed bigram: "<< CandidateWord[i] << "," << CandidateWord[j] << endl;
                }
                else if (Srep(bigram_words) < SigmaRep &&
                    Sread(bigram_words) >= SigmaRead)
                {
                    out << "low srep bigram: " << getSentence(bigram_words) << ",score:" << Srep(bigram_words) << endl;
                }
            }
        }
    }
    LOG(INFO) << "seed bigram size: "<< resultList.size() << endl;
    return true;
}

bool OpinionsManager::GetFinalMicroOpinion(const BigramPhraseContainerT& seed_bigramlist, 
    SentenceContainerT& final_result)
{
    if (seed_bigramlist.empty())
    {return true;}

    NgramPhraseContainerT seedGrams;
    OpinionCandidateContainerT candList;
    for(size_t i = 0; i < seed_bigramlist.size(); i++)
    {
        WordSegContainerT phrase;
        phrase.push_back(seed_bigramlist[i].first);
        phrase.push_back(seed_bigramlist[i].second);

        seedGrams.push_back(phrase);
    }

    for(size_t i = 0; i < seedGrams.size(); i++)
    {
        GenerateCandidates(seedGrams[i], candList, seed_bigramlist);
    }
    LOG(INFO) << "generate candidate opinion finished. =============" << endl;

    LOG(INFO) << " candList.size() = "<< candList.size() << endl;

    std::vector<std::pair<std::string, double> > candString = changeForm(candList);
    sort(candString.begin(), candString.end(), seedPairCmp2);
    out << "before pingzhuang: candidates are " << endl;
    for(size_t i = 0; i < candString.size(); ++i)
    {
        out << "str:" << candString[i].first << ",score:" << candString[i].second << endl;
    }

    final_result = PingZhuang(candString);
    return true;
}

OpinionsManager::SentenceContainerT OpinionsManager::PingZhuang(std::vector<std::pair<std::string,double> >&  candString)
{
    SentenceContainerT tingyongList;
    tingyongList.push_back("那就是");
    tingyongList.push_back("看上去");
    tingyongList.push_back("看起来");
    tingyongList.push_back("因为");
    tingyongList.push_back("虽然");
    tingyongList.push_back("但是");
    tingyongList.push_back("而且");
    tingyongList.push_back("虽然");
    tingyongList.push_back("就是");
    tingyongList.push_back("不但");
    tingyongList.push_back("还是");
    tingyongList.push_back("结果");
    tingyongList.push_back("但");
    tingyongList.push_back("就");
    tingyongList.push_back("如果");
    tingyongList.push_back("另外");

    //out<<"pingzhuang"<<endl;
    SentenceContainerT result;
    for(size_t i=0;i<Z.size();i++)
    {
        string thisString=Z[i];

        boost::algorithm::replace_all(thisString, " ", ",");

        boost::algorithm::replace_all(thisString, "。", ",");
        boost::algorithm::replace_all(thisString, "，", ",");
        boost::algorithm::replace_all(thisString, "！", ",");
        string restString=thisString;
        string todoString;
        int pos=restString.find(",");
        //out<<"i"<<i<<endl;
        while(restString.find(",")!=string::npos)
        {
            todoString=restString.substr(0,pos);
            restString=restString.substr(pos+1);
            double score=0;
            std::vector<std::pair<std::string,double> > numList;
            for(size_t j=0;j<candString.size();j++)
            {  
                // out<<"j"<<j<<endl;
                if(todoString.find(candString[j].first)!=string::npos)
                {
                    score+=candString[j].second;

                    numList.push_back(candString[j]);
                    // break;
                }

            }
            if(score>0.3)
            {
                for(size_t j=0;j<tingyongList.size();j++)
                { 
                    if(todoString.substr(0,tingyongList[j].length())==tingyongList[j])
                    {  
                        todoString=todoString.substr(tingyongList[j].length());
                    }
                }
                result.push_back(todoString);//去除 虽然  但是 而且 就是 不但 但 还是
                for(size_t j=0;j<numList.size();j++)
                { 
                    candString.erase(find(candString.begin(),candString.end(),numList[j]));
                }
            }
            pos=restString.find(",");
        }
    }
    return result;
}

/*
   SentenceContainerT OpinionsManager::PingZhuang( std::vector<SentenceContainerT > candString)
   {
   SentenceContainerT n;
   if(candString.empty())
   {return n;}
   out<<"PingZhuang"<<endl;
   setWindowsize(3);
   SentenceContainerT candVector;
   SentenceContainerT sentence;
//candVector.push_back(candString[0]);
candVector=candString[0];
candString.erase(find(candString.begin(),candString.end(),candString[0]));
out<<"ready"<<endl;
while(!candString.empty())
{   //out<<"candVector:";
for (size_t j=0; j<candVector.size(); j++)
{
// out<<candVector[j];
}
//out<<endl;

for(size_t i=0;i<candString.size();i++)
{
//out<<"add";
for (size_t j=0; j<candString[i].size(); j++)
{
//  out<<candString[i][j];
}
//out<<endl;
if(add(candVector,candString[i]))
{  
candString.erase(find(candString.begin(),candString.end(),candString[i]));
//out<<"success"<<endl;
for (size_t j=0; j<candVector.size(); j++)
{
// out<<candVector[j];
}
//out<<endl;
break;
}
else
{
if(i==candString.size()-1)
{
sentence.push_back(getSentence(candVector));

candVector.clear();
//candVector.push_back(candString[0]);
candVector=candString[0];
candString.erase(find(candString.begin(),candString.end(),candString[0]));
break;
}
}
}

} 
setWindowsize(3);  
return sentence; 


}
*/
bool OpinionsManager::add(SentenceContainerT& candVector, const SentenceContainerT& candString)
{
    int score1=0,score2=0;
    //out<<"add"<<endl;

    SentenceContainerT testVector1=candVector;
    //testVector.push_back(candString);
    testVector1.insert(testVector1.end(),candString.begin(),candString.end());
    //out<<"Srep"<<Srep(testVector1)<<endl;
    //out<<"Sread"<<Sread(testVector1)<<endl;
    if((Srep(testVector1)>SigmaRep-0.25*testVector1.size()&&Sread(testVector1)>SigmaRead-0.25*testVector1.size())&&Srep(testVector1)>SigmaRep)
    {
        score1=Score(testVector1);
        // return true;
    }



    SentenceContainerT testVector2=candVector;
    //testVector.push_back(candString);
    testVector2.insert(testVector2.begin(),candString.begin(),candString.end());
    //out<<"Srep"<<Srep(testVector2)<<endl;
    //out<<"Sread"<<Sread(testVector2)<<endl;
    if((Srep(testVector2)>SigmaRep-0.25*testVector2.size()&&Sread(testVector2)>SigmaRead-0.25*testVector2.size())&&Srep(testVector2)>SigmaRep)
    {
        score2=Score(testVector2);
        // return true;
    }
    if(score1==0&&score2==0)
    {return false;}
    else
    {
        if(score1>score2)
        {
            candVector.insert(candVector.end(),candString.begin(),candString.end());
        }
        else
        {
            candVector.insert(candVector.begin(),candString.begin(),candString.end());
        }
        return true;
    }

}


std::string OpinionsManager::getSentence(const WordSegContainerT& candVector)
{
    string temp="";
    for (size_t j = 0; j < candVector.size(); j++)
    {
        temp += candVector[j];
    }
    return temp;
}

void OpinionsManager::ValidCandidateAndUpdate(const NgramPhraseT& phrase, 
    OpinionCandidateContainerT& candList)
{
    double score = Score(phrase);
    std::vector<std::pair<NgramPhraseT, double> >::iterator it = candList.begin();
    for(; it != candList.end(); ++it)
    {
        if(Sim(phrase, (*it).first) > SigmaSim)
        {
            std::string p1, p2;
            WordVectorToString(p1, phrase);
            WordVectorToString(p2, (*it).first);
            if(score > (*it).second)
            {
                // replace it.
                //LOG(INFO) << "the two phrases is similar and new replace old: ( " << p1
                //    << "," << p2 << endl;
                *it = std::make_pair(phrase, score);
            }
            return;
        }
    }
    candList.push_back(std::make_pair(phrase, score));
}

bool OpinionsManager::NotMirror(const NgramPhraseT& phrase, const BigramPhraseT& bigram)
{
    if(phrase.size() > 3)
    {
        std::string phrasestr;
        WordVectorToString(phrasestr, phrase);
        if(phrasestr.find(bigram.first + bigram.second) != string::npos)
            return false;
        return !(phrase[phrase.size() - 2] == bigram.second);
    }
    else if( phrase.size() == 2)
        return !( (phrase[0] == bigram.second) && (phrase[1] == bigram.first) );
    else if( phrase.size() == 3)
        return !( (phrase[1] == bigram.second) && (phrase[2] == bigram.first) );
    return true;
}

void OpinionsManager::Merge(NgramPhraseT& phrase, const BigramPhraseT& bigram)
{
    phrase.push_back(bigram.second);
}

OpinionsManager::BigramPhraseContainerT OpinionsManager::GetJoinList(const NgramPhraseT& phrase,
    const BigramPhraseContainerT& BigramList)
{
    BigramPhraseContainerT resultList;
    for(size_t i = 0; i < BigramList.size(); i++)
    {
        if(phrase[phrase.size()-1] == BigramList[i].first)
        {
            resultList.push_back( BigramList[i] );
        }
    }
    return resultList;
}

void OpinionsManager::GenerateCandidates(const NgramPhraseT& phrase, 
    OpinionCandidateContainerT& candList, 
    const BigramPhraseContainerT& seedBigrams)
{
    if (phrase.size() > 10 ||Sread(phrase) < SigmaRead || Srep(phrase) < SigmaRep )
    {   
        return;
    }

    ValidCandidateAndUpdate(phrase, candList);

    // find the overlap bigram to prepare combining them.
    BigramPhraseContainerT joinList = GetJoinList(phrase, seedBigrams);
    for (size_t j = 0; j < joinList.size(); j++)
    {
        BigramPhraseT&  bigram = joinList[j];
        if( NotMirror(phrase, bigram) )
        {
            NgramPhraseT new_phrase = phrase;
            Merge(new_phrase, bigram);
            //out << "new phrase generated: " << getSentence(new_phrase) << ",srep:" << Srep(new_phrase) << endl; 
            GenerateCandidates(new_phrase, candList, seedBigrams);
        }
    }
}

std::vector<std::pair<std::string,double> > OpinionsManager::changeForm(const OpinionCandidateContainerT& candList)
{ 
    std::vector<std::pair<std::string, double> > newForm;
    string temp;
    for(size_t i = 0; i < candList.size(); i++)
    {
        WordVectorToString(temp, candList[i].first);
        newForm.push_back(std::make_pair(temp, candList[i].second));
    }
    return newForm;
}

//OpinionsManager::OpinionContainerT OpinionsManager::getMicroOpinions(const OpinionCandidateContainerT& candList)
//{
//    std::vector<SentenceContainerT > MicroOpinions;
//    sort(candList.begin(), candList.end());
//    int length=0;
//    size_t i=0;
//    while(length<SigmaLength&&i<candList.size())
//    {
//        length+=candList[i].first.size();//TODO length()
//        i++;
//        MicroOpinions.push_back(candList[i].first);
//    }
//    return MicroOpinions;
//}

std::vector<std::string> OpinionsManager::get()
{  
    std::cout << "begin do opinion mining...." << std::endl;
    //try
    //{
    //    Ngram_->LoadFile();
    //}
    //catch(std::exception& e)
    //{
    //    std::cout << e.what() << endl;
    //}
    std::cout << "ngram finished...." << std::endl;

    word_freq_records_.clear();
    word_freq_insentence_.clear();
    cached_pmimodified_.clear();
    cached_srep.clear();
    // sread can be reuse in different comment.
    //cached_sread.clear();

    WordSegContainerT wordlist;
    GenCandidateWord(wordlist);
    BigramPhraseContainerT seed_bigramlist;
    GenSeedBigramList(wordlist, seed_bigramlist);
    std::vector<std::string> final_result;
    GetFinalMicroOpinion( seed_bigramlist, final_result);
    if(!final_result.empty())
    {
        // string temp;
        for(size_t i = 0; i < final_result.size(); i++)
        {
            // WordVectorToString(temp,result[i]);
            out << "MicroOpinions:" << final_result[i] << endl;
        }
        out<<"-------------Opinion finished---------------"<<endl;
        out.flush();
    }
    return final_result;
}

};

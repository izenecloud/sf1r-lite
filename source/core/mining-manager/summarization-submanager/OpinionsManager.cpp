#include "OpinionsManager.h"
#include <icma/icma.h>
#include <glog/logging.h>
#include <math.h>   

#define VERY_LOW  -50
#define OPINION_NGRAM_MIN  2
#define MAX_SEED_BIGRAM_RATIO   50
#define REFINE_THRESHOLD  4
#define MAX_SEED_BIGRAM_IN_SINGLE_COMMENT  100

using namespace std;
using namespace cma;

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
    static const UCS2Char  en_dou(','), en_ju('.'), en_gantan('!'),
                 en_wen('?'), en_mao(':'), en_space(' ');
    //static const UCS2Char  ch_ju('。'), ch_dou('，'), ch_gantan('！'), ch_wen('？');
    static const UCS2Char  ch_ju(0x3002), ch_dou(0xff0c), ch_gantan(0xff01), ch_wen(0xff1f), ch_mao(0xff1a), ch_dun(0x3001);
    // 全角标点符号等
    inline bool IsFullWidthChar(UCS2Char c)
    {
        return c >= 0xff00 && c <= 0xffef;
    }

    inline bool IsCJKSymbols(UCS2Char c)
    {
        return c >= 0x3000 && c <= 0x303f;
    }

    inline bool IsCommentSplitChar(UCS2Char c)
    {
        return c == en_dou || c == en_ju || c==en_gantan || 
            c==en_wen || c==en_mao || c == en_space ||
            IsFullWidthChar(c) || IsCJKSymbols(c);
    }

    inline bool IsNeedIgnoreChar(UCS2Char c)
    {
        if(IsCommentSplitChar(c))
            return false;
        return !izenelib::util::UString::isThisChineseChar(c) &&
            !izenelib::util::UString::isThisDigitChar(c) &&
            !izenelib::util::UString::isThisAlphaChar(c);
    }

    inline bool IsCommentSplitStr(const std::string& str, izenelib::util::UString::EncodingType encoding = UString::UTF_8)
    {
        if(str == "." || str == "," || str == "!" || str == "?" || str == ":" || str == " " ||
            str == "。" || str == "，" ||str == "！" || str == "？" || str == "：" ||
            str == "、" )
            return true;
        UString ustr(str, encoding);
        for(size_t i = 0; i < ustr.length(); ++i)
        {
            if(IsCommentSplitChar(ustr[i]))
                return true;
        }
        return false;
    }

    inline bool IsAllChineseStr(const std::string& str, UString::EncodingType encoding = UString::UTF_8)
    {
        UString ustr(str, encoding);
        for(size_t i = 0; i < ustr.length(); ++i)
        {
            if(!izenelib::util::UString::isThisChineseChar(ustr[i]))
                return false;
        }
        return true;
    }

OpinionsManager::OpinionsManager(const string& colPath, const std::string& dictpath)
{
    knowledge_ = CMA_Factory::instance()->createKnowledge();
    knowledge_->loadModel( "utf8", dictpath.c_str());
    //assert(knowledge_->isSupportPOS());
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    //analyzer->setOption(Analyzer::OPTION_TYPE_POS_TAGGING,0);
    //analyzer->setOption(Analyzer::OPTION_ANALYSIS_TYPE,77);
    analyzer_->setKnowledge(knowledge_);
    //analyzer->setPOSDelimiter(posDelimiter.data());
    //Ngram_= new Ngram(colPath);
    string logpath=colPath+"/OpinionsManager.log";
    out.open(logpath.c_str(), ios::out);
    windowsize = 3;
    encodingType_ = UString::UTF_8;
    SigmaRep = 0.1;
    SigmaRead = -6;
    SigmaSim = 0.5;
    SigmaLength = 30;
}

OpinionsManager::~OpinionsManager()
{
    out.close();
    delete knowledge_;
    delete analyzer_;
    //delete Ngram_;
}

void OpinionsManager::setWindowsize(int C)
{
    windowsize=C;
}

void OpinionsManager::setEncoding(izenelib::util::UString::EncodingType encoding)
{
    encodingType_ = encoding;
}

void OpinionsManager::setFilterStr(const std::vector<std::string>& filter_strs)
{
    filter_strs_ = filter_strs;
}

void OpinionsManager::CleanCacheData()
{
    pmi_cache_hit_num_ = 0;
    word_cache_hit_num_ = 0;
    cached_word_insentence_.clear();
    cached_pmimodified_.clear();
    cached_srep.clear();
    SigmaRep_dynamic = CandidateSrepQueueT();
    Z.clear();
    orig_comments_.clear();
}

void OpinionsManager::setComment(const SentenceContainerT& in_sentences)
{
    CleanCacheData();
    //out << "----- original comment start-----" << endl;
    for(size_t i = 0; i < in_sentences.size(); i++)
    { 
        if(Z.size() >= MAX_COMMENT_NUM)
            break;
        izenelib::util::UString ustr(in_sentences[i], encodingType_);
        izenelib::util::UString::iterator uend = std::remove_if(ustr.begin(), ustr.end(), IsNeedIgnoreChar);
        if(uend == ustr.begin())
            continue;
        ustr.erase(uend, ustr.end());
        Z.push_back("");
        ustr.convertString(Z[Z.size() - 1], encodingType_);
        WordSegContainerT allwords;
        stringToWordVector(Z[Z.size() - 1], allwords);
        orig_comments_.push_back(allwords);
        // remove splitter in the sentence, avoid the impact on the srep by them.
        //
        uend = std::remove_if(ustr.begin(), ustr.end(), IsCommentSplitChar);
        if(uend == ustr.begin())
            continue;
        ustr.erase(uend, ustr.end());

        for(size_t j = 0; j < ustr.length(); j++)
        {  
            string strseg;
            string bigramstr;
            ustr.substr(j, 1).convertString(strseg, encodingType_);
            cached_word_insentence_[strseg].set(Z.size() - 1);
            ustr.substr(j, 2).convertString(bigramstr, encodingType_);
            cached_word_insentence_[bigramstr].set(Z.size() - 1);
        }

        Z[Z.size() - 1] = "";
        ustr.convertString(Z[Z.size() - 1], encodingType_);
        //out << Z[Z.size() - 1] << endl;
    }
    //out << "----- original comment end-----" << endl;
    out << "----- total comment num : " << Z.size() << endl;
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

//void OpinionsManager::beginSeg(const string& inSentence, string& outSentence)
//{
//    char* charOut = NULL;
//    analyzer->runWithStream(inSentence.c_str(),charOut);
//    string outString(charOut);
//    outSentence=outString;
//
//}

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

double OpinionsManager::SrepSentence(const std::string& phrase_str)
{
    // do not use cache. cache is computed by single words.
    WordSegContainerT words;
    cma::Sentence single_sen(phrase_str.c_str());
    analyzer_->runWithSentence(single_sen);
    int best = single_sen.getOneBestIndex();
    for(int i = 0; i < single_sen.getCount(best); i++)
    {
        words.push_back(single_sen.getLexicon(best, i));
    }

    size_t n = words.size();
    double sigmaPMI = 0;
    for(size_t i = 0; i < n; i++)
    {
        sigmaPMI += PMIlocal(words, i, windowsize*2);
        // if(windowsize==6)
        // {out<<"PMI:"<<PMIlocal(words,i,windowsize)<<endl;}
    }
    sigmaPMI /= double(n);
    return sigmaPMI;
}

double OpinionsManager::Srep(const WordSegContainerT& words)
{
    if(words.empty())
        return (double)VERY_LOW;

    std::string phrase;
    WordVectorToString(phrase, words);
    CachedStorageT::iterator it = cached_srep.find(phrase);
    if(it != cached_srep.end())
    {
        pmi_cache_hit_num_++;
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

    //if(words.size() == 2 && sigmaPMI < SigmaRep)
    //    sigmaPMI = SigmaRep + 0.01;

    cached_srep[phrase] = sigmaPMI;

    return sigmaPMI;
}

double OpinionsManager::Score(const NgramPhraseT& words)
{
    return  Srep(words) /*+ 0.25*words.size()*/;
}

double OpinionsManager::Sim(const std::string& Mi, const std::string& Mj)
{
    if(Mi.find(Mj) != string::npos ||
        Mj.find(Mi) != string::npos)
    {
        return 0.9;
    }

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

    std::string Mi, Mj;
    WordVectorToString(Mi, wordsi);
    WordVectorToString(Mj, wordsj);
    if(Mi.find(Mj) != string::npos ||
        Mj.find(Mi) != string::npos)
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
    return double(same)/min(double(sizei), double(sizej));//Jaccard similarity
}

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
            pmi_cache_hit_num_++;
            return pit->second;
        }
    }
    int s = Z.size();
    double ret  = log( ( Possib(Wi,Wj) * CoOccurring(Wi,Wj,C) ) * 
        double(s) / ( Possib(Wi)*Possib(Wj) ) ) / log(2);
    if(ret < (double)VERY_LOW)
    {
        cached_pmimodified_[Wi][Wj] = (double)VERY_LOW;
        return (double)VERY_LOW;
    }
    cached_pmimodified_[Wi][Wj] = ret;
    return ret;
}

double OpinionsManager::CoOccurring(const std::string& Wi, const std::string& Wj, int C)
{
    int Poss = 0;
    for(size_t j = 0; j < Z.size(); j++)
    {
        if(cached_word_insentence_.find(Wi) != cached_word_insentence_.end()
            && cached_word_insentence_.find(Wj) != cached_word_insentence_.end())
        {
            if(!cached_word_insentence_[Wi].test(j) || !cached_word_insentence_[Wj].test(j))
            {
                word_cache_hit_num_++;
                continue;
            }
        }
        if(CoOccurringInOneSentence(Wi, Wj, C, Z[j]))
           Poss++; 
    }

    return Poss*Poss;
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
        loci = ustr.find(uwi, loci + uwi.size());
    }
    return false;
}

double OpinionsManager::Possib(const std::string& Wi, const std::string& Wj)
{
    int Poss=0;
    bool wi_need_find = true;
    bool wj_need_find = true;
    if(cached_word_insentence_.find(Wi) != cached_word_insentence_.end())
    {
        word_cache_hit_num_++;
        wi_need_find = false;
    }
    if(cached_word_insentence_.find(Wj) != cached_word_insentence_.end())
    {
        word_cache_hit_num_++;
        wj_need_find = false;
    }
    if(wi_need_find || wj_need_find)
    {
        for(size_t j = 0; j < Z.size(); j++)
        {
            if(wi_need_find)
            {
                size_t loci = Z[j].find(Wi);
                if(loci != string::npos )
                {
                    cached_word_insentence_[Wi].set(j);
                }
            }
            if(wj_need_find)
            {
                size_t locj = Z[j].find(Wj);
                if( locj != string::npos )
                {
                    cached_word_insentence_[Wj].set(j);
                }
            }
        }
    }
    for(size_t j = 0; j < Z.size(); j++)
    {
        if( cached_word_insentence_[Wi].test(j) && cached_word_insentence_[Wj].test(j))
        {
            Poss++;
        }
    }

    return Poss;
}

double OpinionsManager::Possib(const std::string& Wi)
{
    int Poss=0;
    if(cached_word_insentence_.find(Wi) != cached_word_insentence_.end())
    {
        word_cache_hit_num_++;
        return cached_word_insentence_[Wi].count();
    }
    for(size_t j = 0; j < Z.size(); j++)
    {
        size_t loc = Z[j].find(Wi);
        if( loc != string::npos )
        {
            Poss++;
            cached_word_insentence_[Wi].set(j);
        }
    }
    return Poss;
}

bool OpinionsManager::IsNeedFilte(const std::string& teststr)
{
    for(size_t i = 0; i < filter_strs_.size(); ++i)
    {
        if(teststr.find(filter_strs_[i]) != std::string::npos)
            return true;
    }
    return false;
}

static bool FilterBigramByCounter(int filter_counter, const OpinionsManager::WordFreqMapT& word_freq_records,
    const OpinionsManager::BigramPhraseT& bigram)
{
    if(IsCommentSplitStr(bigram.first))
        return false;
    OpinionsManager::WordFreqMapT::const_iterator it = word_freq_records.find(bigram.first + bigram.second);
    if(it != word_freq_records.end())
    {
        return (int)it->second < filter_counter;
    }
    return false;
}

bool OpinionsManager::FilterBigramByPossib(double possib, const OpinionsManager::BigramPhraseT& bigram)
{
    if(IsCommentSplitStr(bigram.first))
        return false;
    if(Possib(bigram.first + bigram.second) > possib)
        return false;
    return true;
}

bool OpinionsManager::GenSeedBigramList(BigramPhraseContainerT& resultList)
{
    // first , get all the frequency in overall and filter the low frequency phrase.
    //
    std::vector<string> bigram_words;
    bigram_words.resize(2);
    WordFreqMapT  word_freq_records;
    for(size_t i = 0; i < orig_comments_.size(); i++)
    {
        int bigram_start = (int)resultList.size();
        int bigram_num_insingle_comment = 0;  // splitter is excluded
        for(size_t j = 0; j < orig_comments_[i].size() - 1; j++)
        {
            if(!IsAllChineseStr(orig_comments_[i][j]))
            {
                bigram_words[0] = bigram_words[1] = ".";
                resultList.push_back(std::make_pair(bigram_words[0], bigram_words[1]));
                continue;
            }
            if(!IsAllChineseStr(orig_comments_[i][j + 1]))
            {
                // last before the splitter. we can pass it.
                continue;
            }

            bigram_words[0] = orig_comments_[i][j];
            bigram_words[1] = orig_comments_[i][j + 1];
            if(IsNeedFilte(bigram_words[0] + bigram_words[1]))
            {
                continue;
            }
            //if( (Srep(bigram_words) >= SigmaRep) )
            {

                string tmpstr = bigram_words[0] + bigram_words[1];
                if(word_freq_records.find(tmpstr) == word_freq_records.end())
                {
                    word_freq_records[tmpstr] = 1;
                }
                else
                {
                    word_freq_records[tmpstr] += 1;
                }

                resultList.push_back(std::make_pair(bigram_words[0], bigram_words[1]));
                ++bigram_num_insingle_comment;
            }
        }
        // refine the seed bigram in a very long single comment.
        if(bigram_num_insingle_comment > MAX_SEED_BIGRAM_IN_SINGLE_COMMENT)
        {
            //out << "seed bigram is too much in phrase: "<< getSentence(orig_comments_[i]) << endl;
            WordPriorityQueue_  topk_seedbigram;
            topk_seedbigram.Init(MAX_SEED_BIGRAM_IN_SINGLE_COMMENT);
            for(int start = bigram_start; start < (int)resultList.size(); ++start)
            {
                std::string bigram_str = resultList[start].first + resultList[start].second;
                if(IsAllChineseStr(bigram_str))
                    topk_seedbigram.insert(std::make_pair(bigram_str, Possib(bigram_str)));
            }
            double possib_threshold = topk_seedbigram.top().second;
            //out << "bigram filter possibility: "<< possib_threshold << endl;
            BigramPhraseContainerT::iterator bigram_it = std::remove_if(resultList.begin() + bigram_start,
                resultList.end(),
                boost::bind(&OpinionsManager::FilterBigramByPossib, this, possib_threshold, _1));
            //out << "bigram filter num: "<< bigram_it - resultList.end() << endl;
            resultList.erase(bigram_it, resultList.end());
        }
        // push splitter at the end of each sentence to avoid the opinion cross two different sentence.
        resultList.push_back(std::make_pair(".", "."));
    }

    // get the top-k bigram counter, and filter the seed bigram by this counter.
    WordPriorityQueue_  topk_words;
    topk_words.Init(min(MAX_SEED_BIGRAM_RATIO*SigmaLength, (double)word_freq_records.size()/2));

    WordFreqMapT::const_iterator cit = word_freq_records.begin();
    while(cit != word_freq_records.end())
    {
        if((int)cit->second > 2)
        {
            topk_words.insert(std::make_pair(cit->first, (int)cit->second));
        }
        ++cit;
    }

    //out << "=== total different bigram:" << word_freq_records.size() << endl;
    int filter_counter = 3;
    if(topk_words.size() > 1)
        filter_counter = max(topk_words.top().second, filter_counter);
    //out << "===== filter counter is: "<< filter_counter << endl;
    BigramPhraseContainerT::iterator bigram_it = std::remove_if(resultList.begin(), resultList.end(),
       boost::bind(FilterBigramByCounter, filter_counter, boost::cref(word_freq_records), _1));
    resultList.erase(bigram_it, resultList.end());
    out << "===== after filter, seed bigram size: "<< resultList.size() << endl;

    if(resultList.size() > MAX_SEED_BIGRAM_RATIO*SigmaLength)
    {
    }
    ///test opinion result
    //

    //ifstream infile;
    //infile.open("/home/vincentlee/workspace/sf1/opinion_result.txt", ios::in);
    //while(infile.good())
    //{
    //    std::string opinion_phrase;
    //    getline(infile, opinion_phrase);
    //    WordSegContainerT  words;
    //    stringToWordVector(opinion_phrase, words);
    //    LOG(INFO) << "phrase: " << getSentence(words) << ", srep:" << Srep(words) << endl;
    //}
    return true;
}

//static inline bool isAdjective(int pos)
//{
//    return pos >=1 && pos <= 4 ;
//}
//
//static inline bool isNoun(int pos)
//{
//    return (pos >= 19 && pos <= 25) || pos == 29;
//}

void OpinionsManager::RefineCandidateNgram(OpinionCandidateContainerT& candList)
{
    // refine the candidate.
    // we assume that a candidate ngram should has at least one or two frequency bigram .
    // and more, we can assume at least one noun phrase or adjective phrase should exist. 
    for(size_t index = 0; index < candList.size(); ++index)
    {
        bool need_refine = false;
        const NgramPhraseT& cand_phrase = candList[index].first;
        double max_possib = 0;
        for(size_t j = 0; j < cand_phrase.size() - 1; ++j)
        {
            max_possib = max(max_possib, Possib(cand_phrase[j] + cand_phrase[j + 1]));
        }
        need_refine = (max_possib < REFINE_THRESHOLD);
        if(need_refine)
        {
            //out << "candidate refined: " << getSentence(candList[index].first) << endl;
            candList[index].second = SigmaRep - 1;
        }
    }
}

void OpinionsManager::GetOrigCommentsByBriefOpinion(std::vector< std::pair<std::string, double> >& candOpinionString)
{
    WordFreqMapT all_orig_split_comments;
    for(size_t i = 0; i < orig_comments_.size(); i++)
    {
        WordSegContainerT  split_comment;
        for(size_t j = 0; j < orig_comments_[i].size(); j++)
        {
            if(IsCommentSplitStr(orig_comments_[i][j]))
            {
                if(!split_comment.empty())
                {
                    all_orig_split_comments[getSentence(split_comment)] += 1;
                    split_comment.clear();
                }
            }
            else
            {
                split_comment.push_back(orig_comments_[i][j]);
            }
        }
        if(!split_comment.empty())
        {
            all_orig_split_comments[getSentence(split_comment)] += 1;
        }
    }
    WordPossibilityMapT  orig_comment_opinion;

    for(size_t i = 0; i < candOpinionString.size(); i++)
    {
        // find the shortest original comment.
        std::string shortest_orig;
        std::string& brief_opinion = candOpinionString[i].first;
        int cnt = 0;
        WordFreqMapT::iterator it = all_orig_split_comments.begin();
        while(it != all_orig_split_comments.end())
        {
            if((*it).first.find(brief_opinion) != std::string::npos)
            {
                cnt += it->second;
                if( shortest_orig.empty() )
                {
                    shortest_orig = it->first;
                }
                else if(SrepSentence(it->first) > SrepSentence(shortest_orig))
                {
                    shortest_orig = it->first;
                }
            }
            ++it;
        }
        if(!shortest_orig.empty())
        {
            if(orig_comment_opinion.find(shortest_orig) == orig_comment_opinion.end())
            {
                orig_comment_opinion[shortest_orig] = candOpinionString[i].second * cnt * brief_opinion.size() / shortest_orig.size();
            }
            else
            {
                orig_comment_opinion[shortest_orig] = max(orig_comment_opinion[shortest_orig], candOpinionString[i].second * cnt * brief_opinion.size() / shortest_orig.size());
            }
        }
        else
        {
            out << "no orig_comment found:" << brief_opinion << endl;
        }
    }
    candOpinionString.clear();
    WordPossibilityMapT::iterator it = orig_comment_opinion.begin();
    while(it != orig_comment_opinion.end())
    {
        if(it->second >= SigmaRep )
        {
            if(SrepSentence(it->first) >= SigmaRep)
                candOpinionString.push_back(std::make_pair(it->first, it->second));
        }
        ++it;
    }
}

void OpinionsManager::recompute_srep(std::vector<std::pair<std::string, double> >& candList)
{
    // compute the srep in sentence. This can get better score for the 
    // meaningful sentence to avoid the bad sentence with good bigrams.
    for(size_t i = 0; i < candList.size(); i++)
    {
        if(candList[i].second >= SigmaRep)
            candList[i].second = SrepSentence(candList[i].first);
    }
}

bool OpinionsManager::GetFinalMicroOpinion(const BigramPhraseContainerT& seed_bigramlist, 
    bool need_orig_comment_phrase, SentenceContainerT& final_result)
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

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    for(size_t i = 0; i < seedGrams.size(); i++)
    {
        GenerateCandidates(seedGrams[i], candList, seed_bigramlist, i);
    }

    gettimeofday(&endtime, NULL);
    int64_t interval = (endtime.tv_sec - starttime.tv_sec)*1000 + (endtime.tv_usec - starttime.tv_usec)/1000;
    if(interval > 10000)
        out << "gen candidate used more than 10000ms " << interval << endl;

    std::cout << "\r candList.size() = "<< candList.size();

    if(candList.size() > SigmaLength/2)
        RefineCandidateNgram(candList);
    std::vector<std::pair<std::string, double> > candOpinionString;
    changeForm(candList, candOpinionString);

    //out << "before recompute_srep: candidates are " << endl;
    //for(size_t i = 0; i < candOpinionString.size(); ++i)
    //{
    //    out << "str:" << candOpinionString[i].first << ",score:" << candOpinionString[i].second << endl;
    //}

    if(need_orig_comment_phrase)
    {
        // get orig_comment from the candidate opinions.
        GetOrigCommentsByBriefOpinion(candOpinionString);
    }
    recompute_srep(candOpinionString);

    //out << "after recompute_srep: candidates are " << endl;
    //for(size_t i = 0; i < candOpinionString.size(); ++i)
    //{
    //    out << "str:" << candOpinionString[i].first << ",score:" << candOpinionString[i].second << endl;
    //}

    sort(candOpinionString.begin(), candOpinionString.end(), seedPairCmp2);
    size_t result_num = min((size_t)SigmaLength, min(2*orig_comments_.size(), candOpinionString.size()));
    for(size_t i = 0; i < result_num; ++i)
    {
        final_result.push_back(candOpinionString[i].first);
    }
    return true;
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
    OpinionCandidateContainerT::iterator it = candList.begin();
    bool can_insert = true;
    bool is_larger_replace_exist = false;

    size_t start = 0;
    size_t remove_end = candList.size();
    while( start < remove_end )
    {
        if(Sim(phrase, candList[start].first) > SigmaSim)
        {
            can_insert = false;
            if(score > candList[start].second)
            {
                //  swap the similarity to the end.
                OpinionCandidateT temp = candList[remove_end - 1];
                candList[remove_end - 1] = candList[start];
                //out << "similarity need removed: " << getSentence(candList[remove_end - 1].first) << endl;
                candList[start] = temp;
                --remove_end;

                continue;
            }
            else
            {
                //out << "similarity larger is exist: " << getSentence(candList[start].first) << ",srep:" << candList[start].second << endl;
                is_larger_replace_exist = true;
            }
        }
        ++start;
    }
    if(!can_insert && (remove_end < candList.size()))
    {
        if(is_larger_replace_exist)
        {
            // just remove all similarity. Because a larger already in the candidate.
            candList.erase(candList.begin() + remove_end, candList.end());
        }
        else
        {
            // replace the similarity with current.
            candList[remove_end] = std::make_pair(phrase, score);
            candList.erase(candList.begin() + remove_end + 1, candList.end());
            //out << "similarity replaced by: " << getSentence(phrase) << endl;
        }
    }
    if(can_insert && score >= SigmaRep && phrase.size() > OPINION_NGRAM_MIN)
    {
        //out << "new added: " << getSentence(phrase) <<  endl;
        if(candList.size() > SigmaLength)
        {
            if(score < SigmaRep_dynamic.top())
            {
                //out << "ignored by full size and low than the smallest: " << getSentence(phrase) << ", " << SigmaRep_dynamic.top() << endl;
                return;
            }
        }
        candList.push_back(std::make_pair(phrase, score));
    }
    SigmaRep_dynamic.push(score);
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
    const BigramPhraseContainerT& bigramlist, int current_merge_pos)
{
    BigramPhraseContainerT resultList;
    if( (size_t)current_merge_pos >= bigramlist.size() - 1 )
        return resultList;
    if(phrase[phrase.size() - 1] == bigramlist[current_merge_pos + 1].first)
    {
        resultList.push_back( bigramlist[current_merge_pos + 1] );
    }
    return resultList;
}

void OpinionsManager::GenerateCandidates(const NgramPhraseT& phrase, 
    OpinionCandidateContainerT& candList, 
    const BigramPhraseContainerT& seedBigrams, int current_merge_pos)
{
    if( (size_t)current_merge_pos >= seedBigrams.size() - 1 )
        return;
    if ( (phrase.size() > OPINION_NGRAM_MIN) && ( Srep(phrase) < SigmaRep ) )
    {   
        return;
    }

    if( phrase.size() > OPINION_NGRAM_MIN)
    {
        ValidCandidateAndUpdate(phrase, candList);
    }

    // find the overlap bigram to prepare combining them.
    BigramPhraseContainerT joinList = GetJoinList(phrase, seedBigrams, current_merge_pos);
    for (size_t j = 0; j < joinList.size(); j++)
    {
        BigramPhraseT&  bigram = joinList[j];
        if( NotMirror(phrase, bigram) )
        {
            NgramPhraseT new_phrase = phrase;
            Merge(new_phrase, bigram);
            //out << "new phrase generated: " << getSentence(new_phrase) << ",srep:" << Srep(new_phrase) << endl; 
            GenerateCandidates(new_phrase, candList, seedBigrams, current_merge_pos + 1);
        }
    }
}

void OpinionsManager::changeForm(const OpinionCandidateContainerT& candList, std::vector<std::pair<std::string,double> >& newForm)
{ 
    string temp;
    for(size_t i = 0; i < candList.size(); i++)
    {
        if(candList[i].second >= SigmaRep)
        {
            WordVectorToString(temp, candList[i].first);
            newForm.push_back(std::make_pair(temp, candList[i].second));
        }
    }
}

std::vector<std::string> OpinionsManager::getOpinion(bool need_orig_comment_phrase)
{
    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    BigramPhraseContainerT seed_bigramlist;
    GenSeedBigramList(seed_bigramlist);

    gettimeofday(&endtime, NULL);
    int64_t interval = (endtime.tv_sec - starttime.tv_sec)*1000 + (endtime.tv_usec - starttime.tv_usec)/1000;
    if(interval > 1000)
        out << "gen seed bigram used more than 1000ms " << interval << endl;

    std::vector<std::string> final_result;
    GetFinalMicroOpinion( seed_bigramlist, need_orig_comment_phrase, final_result);
    if(!final_result.empty())
    {
        for(size_t i = 0; i < final_result.size(); i++)
        {
            out << "MicroOpinions:" << final_result[i] << endl;
        }
        out<<"-------------Opinion finished---------------"<<endl;
        out.flush();
    }
    out << "word/pmi cache hit ratio: " << word_cache_hit_num_ << "," << pmi_cache_hit_num_ << endl;
    CleanCacheData();
    return final_result;
}

};

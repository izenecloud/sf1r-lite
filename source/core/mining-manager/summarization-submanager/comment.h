#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_COMMENT_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_COMMENT_H

#include <icma/icma.h>

#include "Ngram.h"
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <string>
#include <util/PriorityQueue.h>

using std::string;
using std::vector;
using namespace cma;

namespace sf1r
{

class OpinionsManager
{
public:
    typedef std::vector< std::string > SentenceContainerT;
    typedef std::vector< std::string > WordSegContainerT;
    typedef std::pair<std::string, int> WordFreqPairT;
    // record how many sentences the word appeared in.
    typedef boost::unordered_map<std::string, int> WordFreqMapT;

    typedef boost::unordered_map<std::string, double> WordPossibilityMapT;
    typedef boost::unordered_map<std::string, WordPossibilityMapT> WordJoinPossibilityMapT;

    typedef std::map<std::string, double>  CachedStorageT;

    typedef std::pair< std::string, std::string > BigramPhraseT;
    typedef std::vector< BigramPhraseT > BigramPhraseContainerT;
    typedef std::vector< std::string > NgramPhraseT;
    typedef std::vector< NgramPhraseT > NgramPhraseContainerT;
    typedef std::vector< NgramPhraseT > OpinionContainerT;
    typedef std::vector< std::pair< NgramPhraseT, double> > OpinionCandidateContainerT;

    OpinionsManager(const string& colPath);//string enc,string cate,string posDelimiter,
    ~OpinionsManager();
    void beginSeg(const string& inSentence, string& outSentence);
    void stringToWordVector(const std::string& Mi, WordSegContainerT& words);
    void WordVectorToString(std::string& Mi, const WordSegContainerT& words);
    void setWindowsize(int C);
    void setComment(const SentenceContainerT& Z_);
    void setSigma(double SigmaRep_,double SigmaRead_,double SigmaSim_,double SigmaLength_);
    void setEncoding(izenelib::util::UString::EncodingType encoding);
    //score
    //double ScoreRep(const WordSegContainerT& M);
    //double ScoreRep(const std::string& Mi);
    //double ScoreRead(const NgramPhraseT& M);
    //double ScoreRead(const std::string& Mi);
    double Sim(const std::string& Mi, const std::string& Mj);
    double Sim(const NgramPhraseT& wordsi, const NgramPhraseT& wordsj);
    //double getMScore(const WordSegContainerT& M);
    double Srep(const NgramPhraseT& words);
    double Sread(const NgramPhraseT& words);
    double Score(const NgramPhraseT& words);
    //subject
    //bool SubjectLength(const std::vector<std::string>& M);
    //bool SubjectRead(const std::string& Mi);
    //bool SubjectRead(const std::vector<std::string>& M);
    //bool SubjectRep(const std::vector<std::string>& M);
    //bool SubjectRep(const std::string& Mi);
    //bool SubjectSim(const std::vector<std::string>& M);
    //bool Subject(const std::vector<std::string>& M);

    //rep
    double PMIlocal(const WordSegContainerT& words, const int& offset, int C=3);
    double PMImodified(const std::string& Wi, const std::string& Wj, int C=3);
    double Possib(const std::string& Wi, const std::string& Wj);
    double Possib(const std::string& Wi);
    double CoOccurring(const std::string&Wi, const std::string& Wj,int C=3);
    bool CoOccurringInOneSentence(const std::string& Wi,
        const std::string& Wj, int C, const std::string& sentence);
    //algorithm
    bool GenCandidateWord(WordSegContainerT& wordlist);
    bool GenSeedBigramList(const WordSegContainerT& CandidateWord, BigramPhraseContainerT& result_list);
    bool GetFinalMicroOpinion(const BigramPhraseContainerT& seed_bigramlist, SentenceContainerT& final_result);
    void ValidCandidateAndUpdate(const NgramPhraseT& phrase, OpinionCandidateContainerT& candList);
    bool NotMirror(const WordSegContainerT& phrase, const BigramPhraseT& bigram);
    void Merge(NgramPhraseT& phrase, const BigramPhraseT& bigram);
    BigramPhraseContainerT GetJoinList(const WordSegContainerT& phrase, const BigramPhraseContainerT& BigramList);
    void GenerateCandidates(const NgramPhraseT& phrase, OpinionCandidateContainerT& candList, 
        const BigramPhraseContainerT& seedBigrams);
    //OpinionContainerT getMicroOpinions(const OpinionCandidateContainerT& candList);
    std::vector<std::string> get();
    std::vector<std::pair<std::string,double> > changeForm(const OpinionCandidateContainerT& candList);
   
    //std::vector<std::string> PingZhuang( std::vector<std::vector<std::string> > candString);
    std::vector<std::string> PingZhuang( std::vector<std::pair<std::string,double> >& candString);
    bool add(std::vector<std::string>& candVector, const std::vector<std::string>& candString);
    std::string getSentence(const WordSegContainerT& candVector);

private:
    class WordPriorityQueue_ : public izenelib::util::PriorityQueue<WordFreqPairT>
    {
    public:
        WordPriorityQueue_()
        {
        }
        void Init(size_t s)
        {
            initialize(s);
        }
    protected:
        bool lessThan(const WordFreqPairT& o1, const WordFreqPairT& o2) const
        {
            return (o1.second < o2.second);
        }
    };


    double SigmaRep;
    double SigmaRead;
    double SigmaSim;
    double SigmaLength;
    SentenceContainerT Z;
    OpinionContainerT   Micropinion;
    Analyzer* analyzer;
    Knowledge* knowledge;
    CMA_Factory *factory;
    Ngram* Ngram_;
    fstream out;
    int windowsize;
    WordFreqMapT  word_freq_records_;
    CachedStorageT cached_srep;
    CachedStorageT cached_sread;
    WordPossibilityMapT word_freq_insentence_;
    izenelib::util::UString::EncodingType encodingType_;
    WordJoinPossibilityMapT  cached_pmimodified_;
};

};

#endif

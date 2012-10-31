#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_OPINIONMANAGER_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_OPINIONMANAGER_H

#include <icma/icma.h>

#include "Ngram.h"
#include <util/PriorityQueue.h>

#define MAX_COMMENT_NUM  15000

using std::string;
using std::vector;

namespace cma
{
class Analyzer;
class Knowledge;
}

namespace sf1r
{
class OpinionTraining;

class OpinionsManager
{
public:

    struct CustomInt
    {
        CustomInt()
            :inner_(0)
        {
        }
        CustomInt(int in)
            :inner_(in)
        {
        }
        operator int&()
        {
            return inner_;
        }
        operator int() const
        {
            return inner_;
        }
        CustomInt& operator +=(int value)
        {
            inner_ += value;
            return *this;
        }
        int inner_;
    };
    struct CommentScoreT
    {
        double srep_score;
        double occurrence_num;
        CommentScoreT()
            :srep_score(0),
            occurrence_num(0)
        {
        }
        CommentScoreT(double score, double num)
            :srep_score(score),
            occurrence_num(num)
        {
        }
    };
    typedef UString WordStrType;
    // store single comment and split string.
    typedef std::vector< WordStrType > OriginalCommentT;
    typedef std::vector< OriginalCommentT > OriginalCommentContainerT;
    typedef std::vector< WordStrType > SentenceContainerT;
    typedef std::vector< WordStrType > WordSegContainerT;
    typedef std::pair<WordStrType, int> WordFreqPairT;
    // record how many sentences the word appeared in.
    typedef boost::unordered_map<WordStrType, CustomInt> WordFreqMapT;

    typedef boost::unordered_map<WordStrType, std::vector<uint32_t> >  WordInSentenceMapT;
    typedef std::vector< std::vector<uint32_t> >  SingleWordInNgramT;
    typedef boost::unordered_map<WordStrType, double> WordPossibilityMapT;
    typedef boost::unordered_map<WordStrType, WordPossibilityMapT> WordJoinPossibilityMapT;

    typedef boost::unordered_map<WordStrType, double>  CachedStorageT;

    typedef std::pair< WordStrType, WordStrType > BigramPhraseT;
    typedef std::vector< BigramPhraseT > BigramPhraseContainerT;
    typedef std::vector< WordStrType > NgramPhraseT;
    typedef std::vector< NgramPhraseT > NgramPhraseContainerT;
    typedef std::vector< NgramPhraseT > OpinionContainerT;
    typedef std::pair< NgramPhraseT, double > OpinionCandidateT;
    typedef std::vector< OpinionCandidateT > OpinionCandidateContainerT;
    typedef std::pair<CommentScoreT, UString> OpinionCandStringT;
    typedef std::vector< OpinionCandStringT > OpinionCandStringContainerT;

    OpinionsManager(const string& colPath, const std::string& dictpath,
        const string& training_data_path);//string enc,string cate,string posDelimiter,
    ~OpinionsManager();
    void setComment(const SentenceContainerT& Z_);
    std::vector< std::pair<double, WordStrType> > getOpinion(bool need_orig_comment_phrase = true);

    void setWindowsize(int C);
    void setSigma(double SigmaRep_,double SigmaRead_,double SigmaSim_,double SigmaLength_);
    void setEncoding(izenelib::util::UString::EncodingType encoding);
    void setFilterStr(const std::vector<WordStrType>& filter_strs);
    void setSynonymWord(WordSegContainerT& synonyms);
    void CleanCacheData();

private:
    void RecordCoOccurrence(const WordStrType& s, size_t& curren_offset);
    void AppendStringToIDArray(const WordStrType& s, std::vector<uint32_t>& word_ids);
    void recompute_srep(OpinionCandStringContainerT& candList);
    void RefineCandidateNgram(OpinionCandidateContainerT& candList);
    void stringToWordVector(const WordStrType& Mi, WordSegContainerT& words);
    void WordVectorToString(WordStrType& Mi, const WordSegContainerT& words);
    double Sim(const WordStrType& Mi, const WordStrType& Mj);
    double Sim(const NgramPhraseT& wordsi, const NgramPhraseT& wordsj);
    double SimSentence(const WordStrType& sentence_i, const WordStrType& sentence_j);
    double Srep(const NgramPhraseT& words);
    double SrepSentence(const UString& phrase_str);
    double Score(const NgramPhraseT& words);
    //rep
    double PMIlocal(const WordSegContainerT& words, const int& offset, int C=3);
    double PMImodified(const WordStrType& Wi, const WordStrType& Wj, int C=3);
    double Possib(const WordStrType& Wi, const WordStrType& Wj);
    double Possib(const WordStrType& Wi);
    double CoOccurring(const WordStrType& Wi, const WordStrType& Wj,int C=3);
    bool CoOccurringInOneSentence(const WordStrType& Wi,
        const WordStrType& Wj, int C,  int sentence_index);
    //algorithm
    bool GenCandidateWord(WordSegContainerT& wordlist);
    bool GenSeedBigramList(BigramPhraseContainerT& result_list);
    bool GetFinalMicroOpinion(const BigramPhraseContainerT& seed_bigramlist,
       bool need_orig_comment_phrase, std::vector< std::pair<double, WordStrType> >& final_result);
    void ValidCandidateAndUpdate(const NgramPhraseT& phrase, OpinionCandidateContainerT& candList);
    bool NotMirror(const WordSegContainerT& phrase, const BigramPhraseT& bigram);
    void Merge(NgramPhraseT& phrase, const BigramPhraseT& bigram);
    BigramPhraseContainerT GetJoinList(const WordSegContainerT& phrase, const BigramPhraseContainerT& BigramList, int current_merge_pos);
    void GenerateCandidates(const NgramPhraseT& phrase, OpinionCandidateContainerT& candList,
        const BigramPhraseContainerT& seedBigrams, int current_merge_pos);
    void changeForm(const OpinionCandidateContainerT& candList, OpinionCandStringContainerT& newForm);

    std::string getSentence(const WordSegContainerT& candVector);
    bool IsNeedFilter(const WordStrType& teststr);
    void GetOrigCommentsByBriefOpinion(OpinionCandStringContainerT& candOpinionString);
    bool FilterBigramByPossib(double possib, const OpinionsManager::BigramPhraseT& bigram);

    bool IsBeginBigram(const WordStrType& bigram);
    bool IsEndBigram(const WordStrType& bigram);
    bool IsRubbishComment(const WordSegContainerT& words);
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
    typedef std::priority_queue<double, std::vector<double>, greater<double> >  CandidateSrepQueueT;

    double SigmaRep;
    CandidateSrepQueueT SigmaRep_dynamic;
    double SigmaRead;
    double SigmaSim;
    double SigmaLength;
    SentenceContainerT Z;
    fstream out;
    int windowsize;
    CachedStorageT cached_srep;
    WordInSentenceMapT  cached_word_insentence_;
    SingleWordInNgramT  cached_word_inngram_;
    izenelib::util::UString::EncodingType encodingType_;
    WordJoinPossibilityMapT  cached_pmimodified_;
    OriginalCommentContainerT orig_comments_;
    WordSegContainerT  filter_strs_;
    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;
    size_t word_cache_hit_num_;
    size_t pmi_cache_hit_num_;
    size_t valid_cache_hit_num_;
    OpinionTraining* training_data_;
    WordFreqMapT begin_bigrams_;
    WordFreqMapT end_bigrams_;
    WordFreqMapT cached_valid_ngrams_;
    std::vector<int>  sentence_offset_;
    std::map<WordStrType, WordStrType> synonym_map_;
};

};

#endif

#ifndef TEXTSUMMARY_H
#define TEXTSUMMARY_H

#include "ts_types.h"
#include "Graph.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <queue>

using namespace std;

//using namespace std;

namespace sf1r
{
namespace text_summarization
{

class TextSummarization
{
public:
    TextSummarization();
    TextSummarization(size_t size);
    ~TextSummarization();

    int getSummarySize();
    void setSummarySize(size_t size);
    void initialize(const std::vector<Sentence>& input);

    void setGraphProperties(double threshold, int nDirect, double d)
    {
        graph_.setProperties(threshold, nDirect, d);
    }

    //Get text summary by Relevance Measure.
    void computeSummaryRM();
    //Get text summary by Latent Semantic Analysis.
    void computeSummaryLSA();
    //Get text summary by PageRank
    void computeSummaryPR();

    void computeSummaryRM(unsigned int summarySize_,
                          const std::vector<std::vector<unsigned int> >& input,
                          std::vector<unsigned int>& result);

    void getSentencesOrderedBySignificantTerms(
        const std::vector<std::vector<unsigned int> >& input,
        std::vector<unsigned int>& result);

    std::vector<SentenceNO>& getResult()
    {
        return vecTextSummary_;
    }

    //Print the text summary to the file
    void printResult(std::string outFileName);

protected:
    typedef std::map<TermID, int> SIM;
    SIM wordCountMap; //Mapping word to its frequency.

    typedef std::pair<SentenceNO, TermID> ISP; //Make pair for sentence NO. and the terms in the sentence
    typedef std::map<ISP, int> SFM;
    SFM sentenceFeature; //Relate  pair (sentence NO., term) to the number of the term in the sentence.

    // Create weighted term-frequency matrix TermFreqMat for all the
    //sentences and weighted term-frequency matrix TermFreqArr for the whole document.
    double *termFreqMat;
    double *termFreqArr;
    void genTfVetor();

private:
    size_t summarySize_; // The number of the sentence in the text summary;

    std::vector<SentenceNO> vecTextSummary_; //Storage of the text summary.
    std::set<TermID> vocabulary_; //Storage of the terms in article
    std::vector<Sentence> sentenceSet_;//Storage of sentences

    //std::map<TermID, int> wordDistribution_;
    Graph graph_; //For pageRank

};

//Combine the sentence NO. with relevance score, for TextSumRM()*/
struct SentenceRelScore
{
    SentenceNO sentenceNo; //sentence sequence NO.
    double relScore; //Relevance Score of the sentence with theo whole document.
    friend bool operator <(const SentenceRelScore& r1,
                           const SentenceRelScore& r2)
    {
        return (r1.relScore < r2.relScore);
    }
};

//void computeSummaryRM(unsigned int summarySize_,
//        const std::vector<std::vector<unsigned int> >& input,
//        std::vector<unsigned int>& result);
}
}

#endif //TEXTSUMMARY_H

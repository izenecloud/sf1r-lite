#include "svd_wps.h"
#include "TextSummarization.h"
#include <3rdparty/am/rde_hashmap/hash_map.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <map>
#include <set>

namespace sf1r
{
namespace text_summarization
{

TextSummarization::TextSummarization(size_t size)
        : termFreqMat(NULL), termFreqArr(NULL)
{
    summarySize_ = size;
}

TextSummarization::~TextSummarization()
{
    if ( termFreqMat != NULL )
        delete [] termFreqMat;
    if ( termFreqArr != NULL )
        delete [] termFreqArr;
}

int TextSummarization::getSummarySize()
{
    return summarySize_;
}

void TextSummarization::setSummarySize(size_t size)
{
    summarySize_ = size;
}

void TextSummarization::genTfVetor()
{

    int iVocabSize = vocabulary_.size();
    int iSenSize = sentenceSet_.size();

    termFreqMat = new double[iVocabSize*iSenSize];
    termFreqArr = new double[iVocabSize];

    size_t i = 0;
    for (set<TermID>::iterator it=vocabulary_.begin(); it != vocabulary_.end(); it++)
    {
        termFreqArr[i++] = wordCountMap[*it];
        //cout<<wordCountMap[*it] <<" ";
    }

    for (i=0; i<sentenceSet_.size(); i++)
    {
        int j =0;
        for (set<TermID>::iterator it=vocabulary_.begin(); it!=vocabulary_.end();
                it++, j++)
        {
            if (sentenceFeature[make_pair(i,*it)]>0)
            {
                termFreqMat[j*iSenSize+i] = sentenceFeature[make_pair(i,*it)];
                //cout<<sentenceFeature[make_pair(i,*it)]<<" ";
            }
            else
            {
                termFreqMat[j*iSenSize+i] =0;
            }
        }
        //cout<<endl;
    }

}

//get text summary by Relevance Measure.
void TextSummarization::computeSummaryRM()
{

    size_t i;
    size_t s;
    size_t iVocabSize = vocabulary_.size();
    size_t iSenSize = sentenceSet_.size();

    vecTextSummary_.clear();

    bool *isHit = new bool[iSenSize];
    for (i=0; i<iSenSize; i++)
        isHit[i] = 1;
    for (s=0; s<summarySize_ && s<iSenSize; s++)
    {

        priority_queue<SentenceRelScore> rmpq; //using priority_queue for sorting.
        for (i=0; i<iSenSize; i++)
        {
            SentenceRelScore r;
            r.sentenceNo = i;
            r.relScore = 0;
            for (size_t j=0; j<iVocabSize; j++)
            {
                r.relScore += termFreqMat[j*iSenSize+i]*termFreqArr[j];
            }
            //cout<<r.relScore<<endl;
            if (isHit[i])
                rmpq.push(r);
        }

        SentenceRelScore Candiadate = rmpq.top();
        vecTextSummary_.push_back(Candiadate.sentenceNo);

        isHit[Candiadate.sentenceNo] = 0;

        //eliminate all the terms contained in the
        // Candiadate sentences form the document by
        //modifying termFreqArr[i] to 0.
        int TermNo = 0;
        for (set<TermID>::iterator it=vocabulary_.begin(); it!=vocabulary_.end();
                it++, TermNo++)
        {

            if (sentenceFeature[make_pair(Candiadate.sentenceNo, *it)] > 0)
            {
                termFreqArr[TermNo] = 0;
            }

        }
    }
    delete [] isHit;

}

//get text summary by Latent Semantic Analysis.
void TextSummarization::computeSummaryLSA()
{

    size_t m = vocabulary_.size();
    size_t n = sentenceSet_.size();

    vecTextSummary_.clear();

    double *u;
    double *s;
    double *v;

    size_t i, j;

    u = new double[m*m];
    s = new double[m*n];
    v = new double[n*n];

    get_svd_linpack(m, n, termFreqMat, u, s, v); //svd decompositon


    //select the sentences which has the largest index
    //value with the k'th right singular vector.
    for (i=0; i<summarySize_ && i<n; i++)
    {
        j = 0;
        int max = 0;
        double mval = v[j*n+i];

        for (j=1; j<n; j++)
        {
            //cout<<v[i+j*n]<<" ";
            if (v[j*n+i] > mval)
            {
                max = j;
                mval = v[j*n+i];
            }
        }
        //cout<<endl;
        //cout<<max<<" "<<mval<<endl;

        vecTextSummary_.push_back(max);

    }

    delete [] u;
    delete [] s;
    delete [] v;

}

void TextSummarization::computeSummaryPR()
{
    graph_.doTs(sentenceSet_);
    graph_.copyResult(vecTextSummary_, summarySize_);
    /*graph_.printOrderedVertices("dbg.txt"); //for dbg*/

}

void TextSummarization::printResult(string outFileName)
{
    ofstream outf(outFileName.c_str());
    vector<SentenceNO>::iterator it = vecTextSummary_.begin();
    int i = 1;
    for (; it != vecTextSummary_.end(); it++, i++)
    {
        outf<<"["<<i<<"] "<<*it<<endl;
        cout<<"["<<i<<"] "<<*it<<endl;
    }
}

void TextSummarization::initialize(const vector<Sentence>& input)
{
    sentenceSet_ = input;
    if (summarySize_ < 1)
    {
        setSummarySize(sentenceSet_.size());
    }
    for (size_t i=0; i<input.size(); i++)
    {
        for (size_t j=0; j<input[i].size(); j++)
        {
            wordCountMap[input[i][j]]++;
            vocabulary_.insert(input[i][j]);
            sentenceFeature[make_pair(i, input[i][j])]++;
        }
    }
    genTfVetor();
}


void TextSummarization::computeSummaryRM(unsigned int summarySize_,
        const std::vector<std::vector<unsigned int> >& input,
        std::vector<unsigned int>& result)
{
    rde::hash_map<unsigned int, unsigned int> wordCountMap; //Mapping word to its frequency.
    result.clear();
    for (size_t i=0; i<input.size(); i++)
    {
        for (size_t j=0; j<input[i].size(); j++)
        {
            ++wordCountMap[ input[i][j] ];
        }
    }

    typedef std::pair<unsigned int, unsigned int> T;
    size_t iSenSize = input.size();
    std::vector<int> vhit;
    vhit.resize( iSenSize );
    for (unsigned int i=0; i<vhit.size(); i++)
    {
        vhit[i] = 0;
    }

    for (size_t s=0; s<summarySize_ && s<iSenSize; s++)
    {
        priority_queue< T, std::vector<T>, std::less<T> > pq;
        for (size_t i=0; i<input.size(); i++)
        {
            if (vhit[i])
                continue;
            unsigned int score = 0;
            for (size_t j=0; j<input[i].size(); j++)
            {
                score += wordCountMap[ input[i][j] ];
            }
            pq.push( make_pair(score, i) );
        }
        int hit = pq.top().second;
        result.push_back( hit );
        pq.pop();
        for (size_t j=0; j<input[ hit].size(); j++)
        {
            wordCountMap[ input[hit][j] ] = 0;
        }
        vhit[hit] = 1;
    }

}

//get Sentence order by cue phrase evaluation method "Fast Generation of Result Snippet in WebSearch by Andrew Turpin, David Hawking Hugh. E William"

void TextSummarization::getSentencesOrderedBySignificantTerms(
    const std::vector<std::vector<unsigned int> >& input,
    std::vector<unsigned int>& result)
{

    //debug input
    cout << "size of input vectors of sentence TermId " << input.size() << endl;


    rde::hash_map<unsigned int, unsigned int> wordCountMap; //Mapping word to its frequency.
    result.clear();
    for (size_t i=0; i<input.size(); i++)
    {
        for (size_t j=0; j<input[i].size(); j++)
        {
            ++wordCountMap[ input[i][j] ];
        }
    }

    typedef std::pair<double, unsigned int> T;
    size_t iSenSize = input.size();
    std::vector<int> vhit;
    vhit.resize( iSenSize );

    double r_value;
    if (iSenSize < 25)   //value from the paper fp031-Turpi
        r_value = 7-(0.1 * (25-iSenSize));
    else if ((iSenSize >= 25) && (iSenSize <= 40) )
        r_value = 7;
    else
        r_value = 7-(0.1 * (iSenSize-40));


    priority_queue< T, std::vector<T>, std::less<T> > pq;

    double score = 0;
    double sig_score = 0.0;
    unsigned int num_sig_term = 0;
    for (size_t i=0; i<input.size(); i++)
    {
        unsigned int sizeOfSentence = input[i].size();
        for (size_t j=0; j<input[i].size(); j++)
        {
            score =  (double) wordCountMap[ input[i][j] ];
            if (score > r_value)
                num_sig_term += 1;
        }

        //score of sentence is square of number of significant term/total number of sentences
        if (sizeOfSentence > 0)
            sig_score = (num_sig_term*num_sig_term)/sizeOfSentence;

        //cout << "score:  " << score << " sig_score: " <<  sig_score << endl;
        pq.push( make_pair(sig_score, i) );
    }

    for (size_t i=0; i<input.size(); i++)
    {
        int hit = pq.top().second;
        result.push_back( hit );
        pq.pop();
    }
}




/*
 void TextSummarization::initWordDistribution()
 {
 //size_t iVocabSize = vocabulary_.size();
 size_t iSenSize = sentenceSet_.size();

 for(set<string>::iterator it=vocabulary_.begin(); it!=vocabulary_.end(); it++)
 {
 wordDistribution_[*it] = 0;
 for(size_t i=0; i<iSenSize; i++)
 {
 if(sentenceFeature[make_pair(i,*it)]>0)
 {
 wordDistribution_[*it]++;
 }
 }
 }
 }
 */

}
}

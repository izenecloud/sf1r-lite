#include "util.h"

#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/RankDocumentProperty.h>

#include <cmath>

using namespace std;

namespace sf1r
{

double SPLMUtil::kl(const vector<double>& px, const vector<double>& qx)
{
    assert(px.size() == qx.size());
    double val = 0;
    for (unsigned int i = 0 ; i < px.size(); ++i)
    {
        if (px[i] > 0)
        {
            val += px[i] * log(px[i] / qx[i]);
        }
    }
    return val;
}

double **SPLMUtil::getTF(const map<int, int>& wordMap, int s_start, int s_end,
        const int *sentOffs, const int *W)
{
    double **TF = mat_alloc(wordMap.size(), s_end - s_start);
    mat_zero(TF, wordMap.size(), s_end - s_start);
    for (int s = s_start ; s < s_end; ++s)
    {
        for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
        {
            map<int, int>::const_iterator it = wordMap.find(W[i]);
            if (it != wordMap.end())
            {
                ++TF[it->second][s - s_start];
            }
        }
    }
    return TF;
}

double **SPLMUtil::getTFIDF(const map<int, int>& wordMap, int s_start, int s_end,
        const int *sentOffs, int d_start, int d_end, const int *docOffs, const int *W)
{
    double **TF = mat_alloc(wordMap.size(), s_end - s_start);
    mat_zero(TF, wordMap.size(), s_end - s_start);
    map<int, int> IDF;
    getIDF(IDF, s_start, d_start, d_end, sentOffs, docOffs, W);

    for (int s = s_start ; s < s_end; ++s)
    {
        for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
        {
            map<int, int>::const_iterator it = wordMap.find(W[i]);
            if (it != wordMap.end())
            {
                TF[it->second][s - s_start] += log((s_end - s_start) / ( 1 + IDF[W[i]]));
            }
        }
    }
    return TF;
}

// Mapping between word index and the number of sentences containing the word
void SPLMUtil::getISF(map<int, int>& ISF_map, int s_start, int s_end,
        const int *sentOffs, const int *W)
{
    for (int s = s_start; s < s_end; ++s)
    {
        set<int> wordFound; //Keeps track of whether a word appears in the sentence

        for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
        {
            if (wordFound.insert(W[i]).second)
            {
                ++ISF_map[W[i]];
            }
        }
    }
}

// Mapping between word index and the number of documents containing the word
void SPLMUtil::getIDF(map<int, int>& IDF_map, int s_start, int d_start, int d_end,
        const int *sentOffs, const int *docOffs, const int *W)
{
    int s_end = s_start;
    for (int d = d_start; d < d_end; ++d)
    {
        set<int> wordFound;
        while (sentOffs[s_end] < docOffs[d + 1]) ++s_end;

        for (int s = s_start; s < s_end; ++s)
        {
            for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
            {
                if (wordFound.insert(W[i]).second)
                {
                    ++IDF_map[W[i]];
                }
            }
        }
        s_start = s_end;
    }
}

// Mapping between word index and the number of documents in the whole collection containing the word
void SPLMUtil::getICF(map<int, int>& ICF_map, int nColls, const int *sentOffs,
        const int *collOffs, const int *W)
{
    int s_start = 0;
    int s_end = 0;
    for (int c = 0 ; c < nColls; ++c)
    {
        set<int> wordFount;
        while (sentOffs[s_end] < collOffs[c + 1]) ++s_end;

        for (int s = s_start; s < s_end; ++s)
        {
            for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
            {
                if (wordFount.insert(W[i]).second)
                {
                    ++ICF_map[W[i]];
                }
            }
        }
        s_start = s_end;
    }
}

double SPLMUtil::calculateOverlap(const set<int>& set1, const set<int>& set2)
{
    if (set1.empty()) return 0.;
    int c = 0;

    for (set<int>::const_iterator it = set1.begin(); it != set1.end(); ++it)
    {
        if (set2.find(*it) != set2.end()) ++c;
    }
    return double(c) / double(set1.size());
}

bool SPLMUtil::exceedOverlapThreshold(const int *sentOffs, int s, const int *W,
        const set<int>& selected_word_set)
{
    set<int> cur_word_set;
    cur_word_set.insert(&W[sentOffs[s]], &W[sentOffs[s + 1]]);

    double olp = calculateOverlap(cur_word_set, selected_word_set);
    return olp >= THR;
}

void SPLMUtil::getWordMapping(map<int, int>& wordmapping, const set<int>& words)
{
    int wordIndex = 0;
    for (set<int>::const_iterator it = words.begin(); it != words.end(); ++it)
    {
        if (wordmapping.insert(make_pair(*it, wordIndex)).second)
        {
            ++wordIndex;
        }
    }
}

void SPLMUtil::getCollectionWordMapping(map<int, int>& wordmapping, const int *collOffs,
        int c, const int *W)
{
    int wordIndex = 0;
    for (int i = collOffs[c]; i < collOffs[c + 1]; ++i)
    {
        if (wordmapping.insert(make_pair(W[i], wordIndex)).second)
        {
            ++wordIndex;
        }
    }
}

void SPLMUtil::selectSentences(
        vector<UString>& summary_list,
        const Corpus& corpus,
        const int *sentOffs, const int *W,
        const set<pair<double, int> >& result)
{
    set<int> selected_word_set;
    int word_count = 0;
    for (set<pair<double, int> >::const_reverse_iterator it = result.rbegin();
            it != result.rend(); ++it)
    {
        int s = it->second;

        set<int> cur_word_set;
        cur_word_set.insert(&W[sentOffs[s]], &W[sentOffs[s + 1]]);

        double olp = SPLMUtil::calculateOverlap(cur_word_set, selected_word_set);
        if (olp >= THR) continue;

        selected_word_set.insert(cur_word_set.begin(), cur_word_set.end());
        summary_list.push_back(corpus.get_sent(s));

        word_count += summary_list.back().size();
        if (summary_list.size() > SENTENCE_LIMIT)
            break;
    }
}

void SPLMUtil::getRankQueryProperty(RankQueryProperty& rqp, const int *sentOffs, int s,
        const int *W, int documentLength)
{
    map<int, vector<int> > uniqueWordPos;
    for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
    {
        int collectionIndex = W[i];
        if (uniqueWordPos.find(collectionIndex) == uniqueWordPos.end())
        {
            vector<int> newPosVec;
            newPosVec.push_back(i);
            uniqueWordPos[collectionIndex] = newPosVec;
        }
        else
        {
            uniqueWordPos[collectionIndex].push_back(i);
        }
    }

    rqp.setNumDocs(1);
    rqp.setTotalPropertyLength(documentLength);

    int wordIndex = 0;
    map<int, vector<int> >::iterator it = uniqueWordPos.begin();

    for (; it != uniqueWordPos.end(); ++it)
    {

        rqp.addTerm(wordIndex);
        vector<int> posVector = it->second;

        rqp.setTotalTermFreq(posVector.size());
        rqp.setDocumentFreq(uniqueWordPos.size());

        for (unsigned int i = 0; i < posVector.size(); ++i)
        {
            rqp.pushPosition(posVector[i]);
        }

        ++wordIndex;
    }
    rqp.setQueryLength(sentOffs[s + 1] - sentOffs[s]);
}

void SPLMUtil::getRankDocumentProperty(RankDocumentProperty& rdp, int nWords, const int *collOffs,
        int c, const int *W, const map<int, int>& wordMapping)
{
    map<int, vector<int> > uniqueWordPos;
    for (int i = collOffs[c]; i < collOffs[c + 1]; ++i)
    {
        if (wordMapping.find(W[i]) == wordMapping.end())
            continue;
        if (uniqueWordPos.find(W[i]) == uniqueWordPos.end())
        {
            vector<int> newPosVec;
            newPosVec.push_back(i);
            uniqueWordPos[W[i]] = newPosVec;
        }
        else
        {
            uniqueWordPos[W[i]].push_back(i);
        }
    }

    rdp.resize(uniqueWordPos.size() );
    rdp.setDocLength(collOffs[c + 1] - collOffs[c]); //Test

    map<int, vector<int> >::iterator it = uniqueWordPos.begin();
    int wordIndex = 0;
    for (; it != uniqueWordPos.end(); ++it)
    {
        rdp.activate(wordIndex);
        vector<int> posVector = it->second;
        for (unsigned int i = 0; i < posVector.size(); ++i)
        {
            rdp.pushPosition(posVector[i]);
        }
        ++wordIndex;
    }
}

}

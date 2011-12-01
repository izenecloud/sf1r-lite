#include "util.h"
#include "corpus.h"

#include <vector>
#include <map>
#include <set>
#include <iterator>
#include <sstream>

using namespace std;

namespace sf1r
{

void SPLMUtil::to_lowercase(std::string&s)
{
    for (std::string::iterator i = s.begin(); i != s.end(); ++i)
        *i = tolower(*i);
}

double SPLMUtil::kl(vector<double>& px , vector<double>& qx)
{
    assert(px.size() == qx.size());
    double val = 0;
    for (unsigned int i = 0 ; i < px.size(); ++i)
    {
        if (px[i] > 0)
            val += px[i] * log(px[i]/qx[i]);
    }
    return val;
}

double** SPLMUtil::getTF(map<int,int> wordMap, int s_start, int s_end, int* sentOffs, int* W)
{
    double ** TF = mat_alloc_init(wordMap.size(), s_end-s_start, 0.);
    for (int s = s_start ; s < s_end; ++s)
    {
        for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
        {
            ++TF[wordMap[W[i]]][s-s_start];
        }
    }
    return TF;
}

double** SPLMUtil::getTFIDF(map<int,int> wordMap, int s_start, int s_end, int* sentOffs,
        int d_start, int d_end, int* docOffs, int* W)
{
    double ** TF = mat_alloc_init(wordMap.size(), s_end-s_start, 0.);
    map<int,int> IDF = getIDF(s_start, d_start, d_end, sentOffs, docOffs, W);

    for (int s = s_start ; s < s_end; ++s)
    {
        for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
        {
            TF[wordMap[W[i]]][s - s_start] += log((s_end-s_start) / ( 1 + IDF[W[i]]));
        }
    }
    return TF;
}

// Mapping between word index and the number of sentences containing the word
map<int, int> SPLMUtil::getISF(int s_start, int s_end, int* sentOffs, int* W)
{
    map<int, int> ISF_map;
    for (int s = s_start; s < s_end; ++s)
    {
        map<int, bool> wordFound; //Keeps track of whether a word appears in the sentence

        for (int i = sentOffs[s]; i < sentOffs[s+1]; ++i)
        {
            if (wordFound.find(W[i]) == wordFound.end())
            {
                ++ISF_map[W[i]];
                wordFound[W[i]] = true;
            }
        }
    }
    return ISF_map;
}

// Mapping between word index and the number of documents containing the word
map<int, int> SPLMUtil::getIDF(int s_start, int d_start, int d_end, int* sentOffs, int* docOffs, int* W)
{
    map <int, int> IDF_map;
    int s_end = s_start;
    for (int d = d_start; d < d_end; ++d)
    {
        map<int, int> IDF_flag;
        for ( ; sentOffs[s_end] < docOffs[d + 1]; ++s_end);

        for (int s = s_start; s < s_end; ++s)
        {
            for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
            {
                if (IDF_flag.find(W[i]) == IDF_flag.end())
                {
                    ++IDF_map[W[i]];
                    IDF_flag[W[i]] = 1;
                }
            }
        }
        s_start = s_end;
    }
    return IDF_map;
}

// Mapping between word index and the number of documents in the whole collection containing the word
map<int, int> SPLMUtil::getICF(int nColls, int* sentOffs, int* collOffs, int* W)
{
    map <int, int> ICF_map;
    int s_start = 0;
    int s_end = 0;
    for (int c = 0 ; c < nColls; ++c)
    {
        for ( ; sentOffs[s_end] < collOffs[c + 1]; ++s_end);
        map<int, int> IDF_flag;
        for (int s = s_start; s < s_end; ++s)
        {
            for (int i = sentOffs[s]; i < sentOffs[s + 1]; ++i)
            {
                if (IDF_flag.find(W[i]) == IDF_flag.end())
                {
                    ++ICF_map[W[i]];
                    IDF_flag[W[i]] = 1;
                }
            }
        }
        s_start = s_end;
    }
    return ICF_map;
}

double SPLMUtil::calculateOverlap(set<int>& set1, set<int>& set2)
{
    if (set1.size() == 0)
        return 0;
    int c = 0;

    for (set<int>::iterator it = set1.begin(); it != set1.end(); ++it)
        if (set2.find(*it) != set2.end())
            ++c;
    return double(c) / double(set1.size());
}

bool SPLMUtil::exceedOverlapThreshold(int* sentOffs, int s, int* W, set<int> selected_word_set)
{
    set<int> cur_word_set;
    for (int j = sentOffs[s]; j < sentOffs[s + 1]; ++j)
        cur_word_set.insert(W[j]);

    double olp = calculateOverlap(cur_word_set, selected_word_set);
    return olp >= THR;
}

map<int, int> SPLMUtil::getWordMapping(set<int> words)
{
    map<int,int> wordmapping;
    int wordIndex = 0;
    for (set<int>::iterator it = words.begin(); it != words.end(); ++it)
    {
        if (wordmapping.find(*it) == wordmapping.end())
        {
            wordmapping[*it] = wordIndex;
            ++wordIndex;
        }
    }
    return wordmapping;
}

map<int, int> SPLMUtil::getCollectionWordMapping(int* collOffs, int c, int* W)
{
    map<int, int> wordmapping;
    int wordIndex = 0;
    for (int i = collOffs[c]; i < collOffs[c + 1]; ++i)
    {
        if (wordmapping.find(W[i]) == wordmapping.end())
        {
            wordmapping[W[i]] = wordIndex;
            ++wordIndex;
        }
    }
    return wordmapping;
}

void SPLMUtil::selectSentences(string fileName, Corpus corpus, int* sentOffs, int* W,
        set<pair<double,int> > result)
{
    ofstream ofs;
    ofs.open(fileName.c_str());

    set<int> selected_word_set;
    int word_count = 0;
    for (set<pair<double, int> >::reverse_iterator it = result.rbegin(); it != result.rend(); ++it)
    {
        int s = it->second;

        set<int> cur_word_set;
        for (int j = sentOffs[s]; j < sentOffs[s + 1]; ++j)
            cur_word_set.insert(W[j]);

        double olp = SPLMUtil::calculateOverlap(cur_word_set, selected_word_set);
        if (olp >= THR)
            continue;
        selected_word_set.insert(cur_word_set.begin(), cur_word_set.end());
        string sent = corpus.get_sent(s);

        stringstream ss(sent);
        string token;

        while (getline(ss, token, ' '))
        {
            ofs << token << " " ;
            ++word_count;
            if (word_count == WORD_LIMIT)
                break;
        }

        if (word_count == WORD_LIMIT)
            break;
        ofs << endl;

    }
    ofs.close();
}

RankQueryProperty SPLMUtil::getRankQueryProperty(int* sentOffs, int s, int* W, int documentLength)
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
        else {
            uniqueWordPos[collectionIndex].push_back(i);
        }
    }

    RankQueryProperty rqp;
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
    return rqp;
}

RankDocumentProperty SPLMUtil::getRankDocumentProperty(int nWords, int* collOffs, int c, int* W, map<int, int> wordMapping)
{
    map<int, vector<int> > uniqueWordPos;
    for (int i = collOffs[c]; i < collOffs[c + 1]; ++i)
    {
        if (wordMapping.find(W[i]) == wordMapping.end())
        {
            continue;
        }
        if (uniqueWordPos.find(W[i]) == uniqueWordPos.end())
        {
            vector<int> newPosVec;
            newPosVec.push_back(i);
            uniqueWordPos[W[i]] = newPosVec;
        }
        else {
            uniqueWordPos[W[i]].push_back(i);
        }
    }

    RankDocumentProperty rdp;
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
    return rdp;
}

}

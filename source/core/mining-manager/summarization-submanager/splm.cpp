#include "splm.h"
#include "util.h"

#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/RankDocumentProperty.h>
#include <ranking-manager/BM25Ranker.h>
#include <ranking-manager/PlmLanguageRanker.h>
#include <ranking-manager/ClosestPositionTermProximityMeasure.h>

#include <cmath>

using namespace std;

namespace sf1r
{

void SPLM::getSmoothedTfDocument(
        vector<double>& smoothed_tf,
        int c, const int *sentOffs, const int *collOffs,
        const map<int, int>& wordmapping,
        const int *W, int numSentences,
        double **U, double **S, double **TF
)
{
    set<int> chosenIndices;
    int s_start = 0;
    for (int i = collOffs[c]; i < collOffs[c + 1]; i++)
    {
        map<int, int>::const_iterator it = wordmapping.find(W[i]);
        if (it == wordmapping.end()) continue;
        int Ui = it->second;
        if (chosenIndices.find(Ui) != chosenIndices.end())
            continue;
        chosenIndices.insert(Ui);
        double val = 0.;
        for (int j = 0; j < numSentences; j++)
        {
            val += fabs(U[Ui][j] * S[0][j]);
        }

        if (i >= sentOffs[s_start+1])
            s_start++;
        smoothed_tf.push_back(val * (sentOffs[s_start + 1] - sentOffs[s_start]));
    }
}

void SPLM::getSmoothedTfSentence(
        vector<double>& smoothed_tf,
        int s, const int *sentOffs,
        const map<int, int>& wordmapping,
        const int *W, int numSentences,
        double **U, double **S, double **TF
)
{
    set<int> chosenIndices;
    for (int i = sentOffs[s]; i < sentOffs[s + 1]; i++)
    {
        map<int, int>::const_iterator it = wordmapping.find(W[i]);
        if (it == wordmapping.end()) continue;
        int Ui = it->second;
        if (chosenIndices.find(Ui) != chosenIndices.end())
            continue;
        chosenIndices.insert(Ui);
        double val = 0.;
        for (int j = 0; j < numSentences; j++)
        {
            val += fabs(U[Ui][j] * S[0][j]);
        }
        smoothed_tf.push_back(val * (sentOffs[s + 1] - sentOffs[s]));
    }
}

void SPLM::generateSummary(
        const string& pid,
        const string& result_root,
        float mu, float lambda,
        const Corpus& corpus
)
{
    const TermProximityMeasure* termProximityMeasure =
        new MinClosestPositionTermProximityMeasure;
    PlmLanguageRanker plm(termProximityMeasure, mu, lambda);

    int nColls = corpus.ncolls();
    const int *sentOffs = corpus.get_sent_offs();
    const int *docOffs = corpus.get_doc_offs();
    const int *collOffs = corpus.get_coll_offs();
    const int *W = corpus.get_word_seq();

    int s_start = 0;
    int s_end = 0;

    int d_start = 0;
    int d_end = 0;

    for (int c = 0 ; c < nColls; ++c)
    {
        cout << "C = " << c << ", gen: " << result_root + corpus.get_coll_name(c) + "." + pid << endl;

        // Advance s_end and d_end indices
        while (sentOffs[s_end] < collOffs[c + 1]) ++s_end;
        while (docOffs[d_end] < collOffs[c + 1]) ++d_end;

        // Translates original word numbers into numbers in the range (0, # of words in the collection - 1)
        map<int, int> collWordMap;
        SPLMUtil::getCollectionWordMapping(collWordMap, collOffs, c, W);

        //Build the TF matrix
        double **TF = SPLMUtil::getTF(collWordMap, s_start, s_end, sentOffs, W);

        //Calculate SVD
        int numOfSentences = s_end - s_start;
        double **U = mat_alloc(collWordMap.size(), numOfSentences);
        double **S = mat_alloc(1, numOfSentences);
        double **V = mat_alloc(numOfSentences, numOfSentences);
        mat_svd(TF, collWordMap.size(), numOfSentences, U, S[0], V);

        // Get SVD-smoothed term frequencies for the collection
        vector<double> coll_tf;
        getSmoothedTfDocument(coll_tf, c, sentOffs, collOffs, collWordMap, W, numOfSentences, U, S, TF);

        int docI = d_start; // Current document index
        int s_start_doc = s_start; // Sentence index marking start of a document
        int s_end_doc = s_start; // Sentence index marking end of a document
        set<pair<double, int> > result; // (score, sentence index) pairs

        for (int s = s_start ; s < s_end; ++s)
        {
            float sentenceScore = 0.;

            // Construct RankQueryProperty (used for proximity calculation)
            RankQueryProperty rqp;
            SPLMUtil::getRankQueryProperty(rqp, sentOffs, s, W, collOffs[c + 1] - collOffs[c]);

            vector<double> query_tf;
            getSmoothedTfSentence(query_tf, s, sentOffs, collWordMap, W, numOfSentences, U, S, TF);
            vector<double> query_tf_doc = query_tf;

            // Used to only compare occurrences of words that occur in a sentence
            set<int> uniqueWords;
            for (int j = sentOffs[s]; j < sentOffs[s + 1]; ++j)
                uniqueWords.insert(W[j]);

            map<int,int> sentenceWordMapping;
            SPLMUtil::getWordMapping(sentenceWordMapping, uniqueWords);

            // Changing to a new document
            if (sentOffs[s] > docOffs[docI])
            {
                while (sentOffs[s_end_doc] < docOffs[docI + 1]) ++s_end_doc;
                s_start_doc = s;

                if (s_end_doc > s_start_doc)
                {
                    double **TF_doc = SPLMUtil::getTF(collWordMap, s_start_doc, s_end_doc, sentOffs, W);
                    int numOfS_doc = s_end_doc - s_start_doc;

                    // Calculate SVD for a document
                    double **U_doc = mat_alloc(collWordMap.size(), numOfS_doc);
                    double **S_doc = mat_alloc(1, numOfS_doc);
                    double **V_doc = mat_alloc(numOfS_doc, numOfS_doc);

                    mat_svd(TF_doc, collWordMap.size(), numOfS_doc, U_doc, S_doc[0], V_doc);
                    getSmoothedTfSentence(query_tf_doc, s, sentOffs, collWordMap, W, numOfS_doc, U_doc, S_doc,TF_doc);
                }
                docI++;
            }

            for (int docIndex = d_start; docIndex < d_end; docIndex++)
            {
                RankDocumentProperty rdp;
                SPLMUtil::getRankDocumentProperty(rdp, docOffs, docIndex, W, sentenceWordMapping);
                getSmoothedTfDocument(coll_tf, docIndex, sentOffs, docOffs, collWordMap, W, numOfSentences, U, S, TF);
                sentenceScore += plm.getScoreSVD(rqp, rdp, query_tf_doc, query_tf, coll_tf);
            }

            if (sentOffs[s+1] - sentOffs[s] > 5)
                result.insert(make_pair(sentenceScore, s));
        }

        string fileName = result_root + corpus.get_coll_name(c) + "." + pid.data();
        SPLMUtil::selectSentences(fileName, corpus, sentOffs, W, result);

        s_start = s_end;
        d_start = d_end;
    }
}

}

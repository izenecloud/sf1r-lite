#include "splm.h"
#include "splm_util.h"

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
        double **U, double **S, int dim
)
{
    if (dim > numSentences)
        dim = numSentences;

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
        for (int j = 0; j < dim; j++)
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
        double **U, double **S
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
        vector<pair<UString, vector<UString> > >& summary_list,
        const Corpus& corpus,
        int lengthLimit,
        SPLM::SPLM_Alg algorithm,
//      double alpha, double beta, int iter, int topic,
        int D, int E,
        float mu, float lambda
)
{
    const TermProximityMeasure* termProximityMeasure =
        new MinClosestPositionTermProximityMeasure;
    PlmLanguageRanker plm(termProximityMeasure, mu, lambda);

    int nWords = corpus.nwords();
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
        // Advance s_end and d_end indices
        while (sentOffs[s_end] < collOffs[c + 1]) ++s_end;
        while (docOffs[d_end] < collOffs[c + 1]) ++d_end;

        // Translates original word numbers into numbers in the range (0, # of words in the collection - 1)
        map<int, int> collWordMap;
        SPLMUtil::getCollectionWordMapping(collWordMap, collOffs, c, W);

        bool SMOOTH = true;

        int docI = d_start; // Current document index
        int s_start_doc = s_start; // Sentence index marking start of a document
        int s_end_doc = s_start; // Sentence index marking end of a document
        set<pair<double, int> > result; // (score, sentence index) pairs

        int s = s_start;
        vector<vector<double> > smoothedDocuments;
        vector<map<int, vector<double> > > smoothedQueries;

        for(int docIndex = d_start; docIndex < d_end; docIndex++)
        {
            while (sentOffs[s_end_doc] < docOffs[docIndex + 1]) ++s_end_doc;

            int numOfS_doc = s_end_doc - s_start_doc;

            map<int, vector<double> > smoothedQueryMap;
            if (s_end_doc == s_start_doc ||
                    (int) collWordMap.size() <= numOfS_doc ||
                    !SMOOTH)
            {
                smoothedDocuments.push_back(vector<double>());
                smoothedQueries.push_back(smoothedQueryMap);
            }
            else
            {
                double **TF_doc = SPLMUtil::getTF(collWordMap, s_start_doc, s_end_doc, sentOffs, W);
                double **S_doc = mat_alloc(1, numOfS_doc);

                while (sentOffs[s] < docOffs[docIndex + 1]) s++;

                vector<double> smoothedDoc;

                // if SVD
                switch (algorithm)
                {
                case SPLM_SVD:
                    {
                        double **U_doc = mat_alloc(collWordMap.size(), numOfS_doc);
                        double **V_doc = mat_alloc(numOfS_doc, numOfS_doc);

                        mat_svd(TF_doc, collWordMap.size(), numOfS_doc, U_doc, S_doc[0], V_doc);
                        getSmoothedTfDocument(smoothedDoc, docIndex, sentOffs, docOffs,
                                collWordMap, W, numOfS_doc, U_doc, S_doc);

                        mat_free(U_doc);
                        mat_free(V_doc);
                    }
                    break;

                case SPLM_RI:
                    {
                        double **TF_s = SPLM::generateIndexVectors(TF_doc, collWordMap.size(), (s_end_doc - s_start_doc), D, E);

                        for(int m = 0; m < numOfS_doc; m++)
                            S_doc[0][m] = 1.0;

                        getSmoothedTfDocument(smoothedDoc, docIndex, sentOffs, docOffs,
                                collWordMap, W, numOfS_doc, TF_s, S_doc);

                        mat_free(TF_s);
                    }
                    break;

                case SPLM_RI_SVD:
                    {
                        double **U_doc = mat_alloc(collWordMap.size(), numOfS_doc);
                        double **V_doc = mat_alloc(numOfS_doc, numOfS_doc);
                        double **TF_s = generateIndexVectors(TF_doc, collWordMap.size(), s_end_doc - s_start_doc, D, E);

                        mat_svd(TF_s, collWordMap.size(), numOfS_doc, U_doc, S_doc[0], V_doc);
                        getSmoothedTfDocument(smoothedDoc, docIndex, sentOffs, docOffs,
                                collWordMap, W, numOfS_doc, U_doc,S_doc);

                        mat_free(U_doc);
                        mat_free(V_doc);
                        mat_free(TF_s);
                    }
                    break;

//              case SPLM_LDA:
//                  {
//                      set<int> vocabulary = SPLM::getVocabSet(s_end_doc - s_start_doc, collWordMap.size(), TF_doc);
//                      map<int, int> vocabMap;
//                      int vocabIndex = 0;
//                      vector<std::string> vocabVec;
//                      for (set<int>::iterator iter = vocabulary.begin(); iter != vocabulary.end(); ++iter)
//                      {
//                          stringstream ss;
//                          ss << *iter;
//                          vocabVec.push_back(ss.str());
//                          vocabMap[*iter] = vocabIndex;
//                          vocabIndex++;
//                      }
//                      vector<vector<pair<size_t,size_t> > > matrixVec = getMatrixVec(
//                              s_end_doc - s_start_doc, collWordMap.size(), TF_doc, vocabMap);

//                      BOW *bw = new BOW();
//                      bw->loadCorpusFileVectors(vocabVec, matrixVec);
//                      LDA *ldaModel = new LDA;
//                      ldaModel->loadCorpus(bw);
//                      ldaModel->inference(iter, alpha, beta, topic);
//                      double **theta, **phi;
//                      ldaModel->exportModel(theta, phi, (size_t) (s_end_doc - s_start_doc), (size_t) collWordMap.size(), (size_t) topic);
//                      double **phi_T = mat_transpose(phi, topic, collWordMap.size());

//                      for(int m = 0; m < numOfS_doc; m++)
//                          S_doc[0][m] = 1.0;

//                      getSmoothedTfDocument(smoothedDoc, docIndex, sentOffs, docOffs,
//                              collWordMap, W, topic, phi_T, S_doc);

//                      mat_free(phi_T);
//                  }
//                  break;

                default:
                    break;
                }
                smoothedDocuments.push_back(smoothedDoc);

                mat_free(S_doc);
                mat_free(TF_doc);
            }
            s_start_doc = s_end_doc;
        }

        bool activated =false;
        for (s = s_start ; s < s_end; ++s)
        {
            float sentenceScore = 0.;

            // Construct RankQueryProperty (used for proximity calculation)
            RankQueryProperty rqp;
            SPLMUtil::getRankQueryProperty(rqp, sentOffs, s, W, collOffs[c+1] - collOffs[c]);

            vector<double> query_tf;
            vector<double> query_tf_doc;

            if (!activated)
            {
                while (sentOffs[s_end_doc] < docOffs[docI + 1]) ++s_end_doc;

                double **TF_doc = SPLMUtil::getTF(collWordMap, s_start_doc, s_end_doc, sentOffs, W);
                s_end_doc = s_start;

                for (int i = 0; i < s_end - s_start; i++)
                {
                    query_tf_doc.push_back(.01);
                }
                activated = true;

                mat_free(TF_doc);
            }

            // Used to only compare occurrences of words that occur in a sentence
            set<int> uniqueWords;
            uniqueWords.insert(&W[sentOffs[s]], &W[sentOffs[s + 1]]);

            map<int,int> sentenceWordMapping;
            SPLMUtil::getWordMapping(sentenceWordMapping, uniqueWords);

            // Changing to a new document
            if (sentOffs[s] > docOffs[docI])
            {
                docI++;
                while (sentOffs[s_end_doc] < docOffs[docI]) ++s_end_doc;

                s_start_doc = s;
            }

            for (int docIndex = d_start; docIndex < d_end; docIndex++)
            {
                RankDocumentProperty rdp;
                SPLMUtil::getRankDocumentProperty(rdp, nWords, docOffs, docIndex, W, sentenceWordMapping);

                vector<double> temp;
                vector<double> smoothedDocument;
                if (SMOOTH == true)
                    smoothedDocument = smoothedDocuments[docIndex - d_start];

                //sentenceScore += plm.getScoreSVD(rqp, rdp, query_tf_doc, temp, smoothedDocument);
                if (algorithm == SPLM_NONE)
                    sentenceScore += plm.getScore(rqp, rdp);
                else
                    sentenceScore += plm.getScoreSVD(rqp, rdp, query_tf_doc, temp, smoothedDocument);
            }

            if (sentOffs[s + 1] - sentOffs[s] > lengthLimit)
                result.insert(make_pair(sentenceScore, s));
        }

        summary_list.push_back(make_pair(UString(), vector<UString>()));
        summary_list.back().first.assign(corpus.get_coll_name(c));
        SPLMUtil::selectSentences(summary_list.back().second, corpus, sentOffs, W, result);

        s_start = s_end;
        d_start = d_end;
    }
}

void SPLM::getVocabSet(set<int>& vocabulary, int row, int col, double **TF)
{
    for (int i = 0; i < col; i++)
    {
        for (int j = 0; j < row; j++)
        {
            if (TF[i][j] > 0)
            {
                vocabulary.insert(i);
                break;
            }
        }
    }
}

double **SPLM::generateIndexVectors(double **TF, int row, int col, int D, int E)
{
    D = col;

    double **indexVec = mat_alloc(col, D);
    for (int i = 0; i < col; i++)
    {
        for (int j = 0; j < D; j++)
        {
            indexVec[i][j] = rand() % 3 - 1;
        }
    }

    double **smoothedTF = mat_alloc(row, D);
    mat_mul(TF, row, col, indexVec, col, D, smoothedTF);
    mat_free(indexVec);

    return smoothedTF;
}

}

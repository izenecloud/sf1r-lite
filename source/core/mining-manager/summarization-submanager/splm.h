///
/// @file splm.h
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SPLM_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SPLM_H_


#include "corpus.h"
#include "svd/d-mat2d.h"
#include "svd/svd.h"

#include <map>

namespace sf1r
{

class SPLM
{
public:
    enum SPLM_Alg
    {
        SPLM_SVD,
        SPLM_RI,
        SPLM_RI_SVD,
//      SPLM_LDA,
        SPLM_NONE
    };

    ///
    /// @brief Generates SVD-smoothed term frequencies for a document (or a collection)
    /// @param c Index of the document (or a collection) whose term frequencies will be smoothed
    /// @param sentOffs Sentence offsets
    /// @param collOffs Document offsets (or collection offsets)
    /// @param wordmapping Mapping between a word ID and a number in the minimized range
    /// @param W Word IDs
    /// @param numSentences Number of sentences
    /// @param U U matrix from SVD calculation
    /// @param S Sigma matrix from SVD calculation
    /// @param dim Reduced dimension size (for smoothing)
    ///
    static void getSmoothedTfDocument(
            std::vector<double>& smoothed_tf,
            int c, const int *sentOffs, const int *collOffs,
            const std::map<int, int>& wordmapping,
            const int *W, int numSentences,
            double **U, double **S, int dim = 20
    );

    ///
    /// @brief Generates SVD-smoothed term frequencies for a sentence
    /// @param c Index of the sentence whose term frequencies will be smoothed
    /// @param sentOffs Sentence offsets
    /// @param wordmapping Mapping between a word ID and a number in the minimized range
    /// @param W Word IDs
    /// @param numSentences Number of sentences
    /// @param U U matrix from SVD calculation
    /// @param S Sigma matrix from SVD calculation
    ///
    static void getSmoothedTfSentence(
            std::vector<double>& smoothed_tf,
            int s, const int *sentOffs,
            const std::map<int, int>& wordmapping,
            const int *W, int numSentences,
            double **U, double **S
    );

    ///
    /// @brief Generates summary for a corpus, using the SPLM algorithm
    /// @param pid This parameter is only relevant when running ROUGE testing
    /// @param result_root Directory to store the generated summary
    /// @param mu Mu parameter for PLM
    /// @param lambda Lambda parameter for PLM
    /// @param corpus Corpus to be summarized
    ///
    static void generateSummary(
            std::vector<std::pair<UString, std::vector<UString> > >& summary_list,
            const Corpus& corpus,
            int lengthLimit = 0,
            SPLM_Alg algorithm = SPLM_SVD,
//          double alpha = 0.038, double beta = 0.1, int iter = 1002, int topic = 10,
            int D = 400, int E = 200,
            float mu = 2000, float lambda = 1000
    );

    static void getVocabSet(std::set<int>& vocabulary, int row, int col, double **TF);

    static double **generateIndexVectors(double **TF, int row, int col, int D, int E);
};

}

#endif /* SF1R_MINING_MANAGER_SUMMARIZATION_SPLM_H_ */

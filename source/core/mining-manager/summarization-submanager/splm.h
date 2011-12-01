///
/// @file splm.h
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SPLM_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_SPLM_H_


#include "corpus.h"
#include "svd/svd.h"
#include "svd/d-mat2d.h"

#include <map>

namespace sf1r
{

class SPLM
{
public:
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
    /// @param TF Term frequency matrix
    ///
    static void getSmoothedTfDocument(
            std::vector<double>& smoothed_tf,
            int c, int* sentOffs, int* collOffs,
            std::map<int,int> wordmapping,
            int* W, int numSentences,
            double ** U, double **S, double **TF
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
    /// @param TF Term frequency matrix
    ///
    static void getSmoothedTfSentence(
            std::vector<double>& smoothed_tf,
            int s, int* sentOffs,
            std::map<int,int> wordmapping,
            int* W, int numSentences,
            double ** U, double **S, double ** TF
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
            const std::string& pid,
            const std::string& result_root,
            float mu, float lambda,
            Corpus corpus
    );

};

}

#endif

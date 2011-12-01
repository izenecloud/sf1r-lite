///
/// @file util.h
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_UTIL_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_UTIL_H_

#include "corpus.h"
#include "svd/d-mat2d.h"

#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/RankDocumentProperty.h>

#include <set>
#include <map>
#include <sstream>

#define THR  0.3
#define WORD_LIMIT 100

namespace sf1r
{

class SPLMUtil
{
public:

    ///
    /// @brief Calculates the KL divergence between probability distributions px and qx
    ///
    static double kl(const std::vector<double>& px, const std::vector<double>& qx);

    ///
    /// @brief Calculates the term frequency matrix for a text selection
    /// @param wordMap Maps word ID to a number in the minimized range
    /// @param s_start Start sentence index of the text selection
    /// @param s_end End sentence index of the text selection
    /// @param sentOffs Sentence offsets
    /// @param W Word IDs
    ///
    static double** getTF(const std::map<int,int>& wordMap, int s_start,
            int s_end, int* sentOffs, int* W);

    ///
    /// @brief Calculates tf-idf matrix for a text selection
    /// @param wordMap Maps word ID to a number in the minimized range
    /// @param s_start Start sentence index of the text selection
    /// @param s_end End sentence index of the text selection
    /// @param d_start Start document index of the document
    /// @param d_end End document index of the document
    /// @param sentOffs Sentence offsets
    /// @param docOffs Document offsets
    /// @param W Word IDs
    ///
    static double** getTFIDF(const std::map<int,int>& wordMap, int s_start, int s_end, int* sentOffs,
            int d_start, int d_end, int* docOffs, int* W);

    ///
    /// @brief Retrieves ISF: mapping between word index and the number of sentences containing the word
    ///
    static void getISF(std::map<int, int>& ISF_map, int s_start, int s_end, int* sentOffs, int* W);

    ///
    /// @brief Retrieves IDF: mapping between word index and the number of documents containing the word
    ///
    static void getIDF(std::map<int, int>& IDF_map, int s_start, int d_start,
            int d_end, int* sentOffs, int* docOffs, int* W);

    ///
    /// @brief Retrieves ICF: mapping between word index and the number of documents in the whole collection containing the word
    ///
    static void getICF(std::map<int, int>& ICF_map, int nColls, int* sentOffs,
            int* collOffs, int* W);

    ///
    /// @brief Calculates percentage overlap between two sets
    ///
    static double calculateOverlap(std::set<int>& set1, std::set<int>& set2);

    ///
    /// @brief Examines whether a sentence in question exceeds the overlap threshold
    ///
    static bool exceedOverlapThreshold(int* sentOffs, int s, int* W,
            std::set<int> selected_word_set);

    ///
    /// @brief Gives a mapping from a word ID to a number in a minimized range
    ///
    static void getWordMapping(std::map<int, int>& wordmapping, std::set<int> words);

    ///
    /// @brief Gives a word mapping for the entire collection
    ///
    static void getCollectionWordMapping(std::map<int, int>& wordmapping, int* collOffs, int c, int* W);

    ///
    /// @brief Selects sentences for the summary set
    ///
    static void selectSentences(std::string fileName, Corpus corpus, int* sentOffs, int* W,
            std::set<std::pair<double,int> > result);

    ///
    /// @brief Converts a sentence into RankQueryProperty (for PLM calculation)
    ///
    static void getRankQueryProperty(RankQueryProperty& rqp, int* sentOffs, int s,
            int* W, int documentLength);

    ///
    /// @brief Converts a document into RankDocumentProperty (for PLM calculation)
    ///
    static void getRankDocumentProperty(RankDocumentProperty& rdp, int nWords,
            int* collOffs, int c, int* W, std::map<int, int> wordMapping);
};

}

#endif

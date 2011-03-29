#ifndef CHARIKAR_ALGO_H
#define CHARIKAR_ALGO_H
/**
 * @file charikar_algo.h
 * @brief impliments Charikar's algorith
 *
 * CharikarAlgorithm implements random projection based near-duplicate
 * algorithm found in Charikar's paper, "Similarity Estimation Techniques
 * from Rounding Algorithms". This algorithm implementation is based
 * on Henzinger in "Finding Near-Duplicate Web Pages: A Large-Scale
 * Evaluation of Algorithms"
 * Given a document, represented as a sequence of token strings,
 * a document signature is nBit vector where all of the sum
 * of random projections of document tokens are later cast to etiher
 * 1 (when positive) and 0 (when negative).
 * Given the two document signature objects, the number of
 * agreed-on bits represents the cosine similarity between the two
 * vectors and any number of bit matches higher than a threshold
 * is construted as a near-duplicate.
 *
 * @author Kevin Hu
 * @date 11/09/09
 */

#include "rand_proj.h"
#include "rand_proj_gen.h"

#include <util/CBitArray.h>
#include <vector>


namespace sf1r{
/**
   @class CharikarAlgo
 * @brief Charikar alogrithm
 */
class CharikarAlgo
{
  
private:
    
    RandProjGen rpEngine;//!< random projection engine     */
    int nDimensions; //!< dimensions number
    double threshold;//!< similarity threshold
public:
    static const double DEFAULT_THRESHOLD_VALUE = 0.97;//!< default dimensions number
    static const int DEFAULT_NUM_DIMENSIONS = 384;//!< default threshold value
public:
    /**
     * @brief constructor of CharikarAlgo, initial members
     *
     * @param nDim dimensions number
     * @param tvalue threshold value
     */
    inline CharikarAlgo(const char* filenm,
                        int nDim=CharikarAlgo::DEFAULT_NUM_DIMENSIONS,
                        double tvalue=CharikarAlgo::DEFAULT_THRESHOLD_VALUE)
      :rpEngine(filenm, nDim), nDimensions(nDim), threshold(tvalue) { }
    
    inline CharikarAlgo(const char* filenm, double tvalue):
      rpEngine(filenm, DEFAULT_NUM_DIMENSIONS), nDimensions(DEFAULT_NUM_DIMENSIONS), threshold(tvalue) { }
    /**
     * @brief disconstructor
     */
    inline virtual ~CharikarAlgo() {}
public:
    /**
     * @brief get dimensions number
     *
     * @return dimensions number
     */
    inline int num_dimensions() { return nDimensions; }
    /**
     * @brief get score of two documents
     *
     * @param sig1 first signature
     * @param sig2 second signature
     *
     * @return the duplicate score
     */
    //int neardup_score(NearDuplicateSignature& sig1, NearDuplicateSignature& sig2);
    /**
     * @brief get score of 2 documents
     *
     * @param sig1 first bit array
     * @param sig2 second bit array
     *
     * @return the duplicate score
     */
    //int neardup_score(const izenelib::util::CBitArray& sig1, const izenelib::util::CBitArray& sig2);

    /* void generate_document_signature(const wiselib::DynamicArray<wiselib::ub8>& docTokens, izenelib::util::CBitArray& bitArray) */
/*     { */
/*     } */
    

    /**
     * @brief generate document signature from document string, not store the document tokens
     *
     * @param[in] document document content
     * @param[out] bitArray document signature
     */
    /* void generate_document_signature(const wiselib::YString& document, izenelib::util::CBitArray& bitArray) */
/*     { */
/*     } */
    

    /**
     * @brief generate document signature
     *
     * @param[in] docTokens term id sequence of a document
     * @param[out] bitArray sequent of bits as a document signature
     */
    void generate_document_signature(const std::vector<unsigned int>& docTokens, izenelib::util::CBitArray& bitArray)
    {
    }
    

 /**
     * @brief generate signature from a vector
     *
     * @param[in] docTokens input source, a term id array
     * @param[out] bitArray signature
     */
    void generate_document_signature(const std::vector<std::string>& docTokens, izenelib::util::CBitArray& bitArray);
    /**
     * @brief check if is near duplicate
     *
     * @param neardupScore duplicate score to check
     *
     * @return true, if is near duplicate, false else.
     */
    /* bool is_neardup(float neardupScore) { */
/*         return (double)neardupScore/(double)nDimensions > threshold; */
/*     } */

    /**
     * @brief set threshold
     *
     * @param th target threshold to set
     */
    void set_threshold(float th= DEFAULT_THRESHOLD_VALUE) {
        threshold = th;
    }
};
}

#endif

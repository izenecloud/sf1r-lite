/**
 * @file    BM25Ranker.cpp
 * @author  Jinglei Zhao & Dohyun Yun
 * @brief   BM25Ranker Implements the BM25 probabilistic model for ranking.
 * @version 1.1
 *
 * - Copyright   : iZENESoft
 * - Log
 *   - Optimization factors
 *     1. The key of documentItem.termFreqMap or PosMap is string. It should
 *        be integer.
 *   - 2009-09-24 Ian Yang <doit.ian@gmail.com>
 *     - Query Adjustment. Rank document item one by one.
 *   - 2010-04-07 Jia Guo
 *     - Corrent the formula.
 */
#include "BM25Ranker.h"

#include <util/get.h>
#include <iostream>
#include <cmath>

namespace sf1r {

void BM25Ranker::setupStats(const RankQueryProperty& queryProperty)
{
    idfParts_.resize(queryProperty.size());

    for (std::size_t i = 0; i != queryProperty.size(); ++i)
    {
        float df = queryProperty.documentFreqAt(i);
        float nDocs = queryProperty.getNumDocs();

        float idfPart = std::log(
            (nDocs - df + 0.5F)
            / (df + 0.5F)
        );
        if( idfPart < minIdf_ )
        {
            idfPart = minIdf_;
        }
        idfParts_[i] = idfPart;
    }
}

float BM25Ranker::getScore(
    const RankQueryProperty& queryProperty,
    const RankDocumentProperty& documentProperty
) const
{
    float score = 0.0F;
    if (0 == queryProperty.getTotalPropertyLength())
    {
        return score;
    }

    for (std::size_t i = 0; i != queryProperty.size(); ++i)
    {
        float tfInDoc = documentProperty.termFreqAt(i);
        float tfInQuery = queryProperty.termFreqAt(i);

        // If the term exists
        if(tfInQuery > 0.0F && tfInDoc > 0.0F)
        {
            uint32_t propLength = documentProperty.docLength();
            float avgPropLength = queryProperty.getAveragePropertyLength();
            float denominatorTF_LN = k1_ * (
                b_ * propLength
                / avgPropLength
                + (1 - b_)
            ) + tfInDoc;

            float tf_LNPart = (k1_ + 1) * tfInDoc / denominatorTF_LN;
            float qtfPart = (k3_ + 1) * tfInQuery / (k3_ + tfInQuery);
            score += idfParts_[i] * qtfPart * tf_LNPart;
            
//             std::cout<<"[1] "<<tfInDoc<<" "<<tfInQuery<<" "<<df<<" "<<nDocs<<" "<<idfParts_[i]<<" "<<propLength<<" "<<avgPropLength<<" "<<b_<<" "<<k1_<<" "<<score<<std::endl;
        }
        
    }
    return score;
} // BM25Ranker::getScore

BM25Ranker* BM25Ranker::clone() const
{
    return new BM25Ranker(*this);
}

} // namespace sf1r

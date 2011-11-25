/*
 * TwoStageSmoother.cpp
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#include "TwoStageSmoother.h"

TwoStageSmoother::TwoStageSmoother(
        uint32_t docTermCount,
        uint32_t collectionTermCount,
        double bkgCoefficient,
        double dirichletCoefficient)
    : docTermCount_(docTermCount)
    , collectionTermCount_(collectionTermCount)
    , bkgCoefficient_(bkgCoefficient)
    , dirichletCoefficient_(dirichletCoefficient)
{
}

TwoStageSmoother::~TwoStageSmoother()
{
    // TODO Auto-generated destructor stub
}

double TwoStageSmoother::computeSmoothedProb(uint32_t dtf, uint32_t qtf)
{
    // TODO Auto-generated destructor stub
    double docCollectionProb = dirichletCoefficient_* qtf / collectionTermCount_;
    double queryBackgroundProb = bkgCoefficient_* qtf / collectionTermCount_;

    return ((1 - bkgCoefficient_) * (dtf + docCollectionProb) / (docTermCount_ + dirichletCoefficient_) + queryBackgroundProb);

}

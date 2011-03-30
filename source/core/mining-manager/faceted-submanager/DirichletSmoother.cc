/*
 * DirichletSmoother.cpp
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#include "DirichletSmoother.h"
DirichletSmoother::DirichletSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
                                     double bkgCoefficient, double dirichletCoefficient):
        docTermCount_(docTermCount),collectionTermCount_(collectionTermCount)
        ,bkgCoefficient_(bkgCoefficient),dirichletCoefficient_(dirichletCoefficient)
{
}

DirichletSmoother::~DirichletSmoother()
{
    // TODO Auto-generated destructor stub
}
double DirichletSmoother::computeSmoothedProb(uint32_t dtf,uint32_t qtf)
{

    double collectionProb=dirichletCoefficient_* qtf/collectionTermCount_;
    return (dtf+collectionProb)/(docTermCount_+dirichletCoefficient_);

}

/*
 * JMSmoother.cpp
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#include "JMSmoother.h"

JMSmoother::JMSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
                       double bkgCoefficient, double dirichletCoefficient):
        docTermCount_(docTermCount),collectionTermCount_(collectionTermCount)
        ,bkgCoefficient_(bkgCoefficient),dirichletCoefficient_(dirichletCoefficient)
{
}

JMSmoother::~JMSmoother()
{
    // TODO Auto-generated destructor stub
}

double JMSmoother::computeSmoothedProb(uint32_t dtf,uint32_t qtf)
{
    // TODO Auto-generated destructor stub
    double collectionProb=bkgCoefficient_* qtf/collectionTermCount_;
    return dtf*(1-bkgCoefficient_)/docTermCount_+collectionProb;

}


/*
 * TwoStageSmoother.h
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#ifndef TWOSTAGESMOOTHER_H_
#define TWOSTAGESMOOTHER_H_
typedef unsigned int uint32_t;
class TwoStageSmoother
{
public:
    TwoStageSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
                     double bkgCoefficient, double dirichletCoefficient);
    virtual ~TwoStageSmoother();
    double computeSmoothedProb(uint32_t dtf,uint32_t qtf);
private:
    double bkgCoefficient_;
    double dirichletCoefficient_;
    uint32_t docTermCount_;
    uint32_t collectionTermCount_;

};
#endif /* TWOSTAGESMOOTHER_H_ */

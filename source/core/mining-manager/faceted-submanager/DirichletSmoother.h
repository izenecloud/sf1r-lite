/*
 * DirichletSmoother.h
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#ifndef DIRICHLETSMOOTHER_H_
#define DIRICHLETSMOOTHER_H_
typedef unsigned int uint32_t;
class DirichletSmoother
{
public:
    DirichletSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
                      double bkgCoefficient, double dirichletCoefficient);
    virtual ~DirichletSmoother();
    double computeSmoothedProb(uint32_t dtf,uint32_t qtf);
private:

    uint32_t docTermCount_;
    uint32_t collectionTermCount_;
    double bkgCoefficient_;
    double dirichletCoefficient_;
};

#endif /* DIRICHLETSMOOTHER_H_ */

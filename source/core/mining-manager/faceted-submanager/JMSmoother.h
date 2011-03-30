/*
 * JMSmoother.h
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#ifndef JMSMOOTHER_H_
#define JMSMOOTHER_H_
typedef unsigned int uint32_t;
class JMSmoother
{
public:
    JMSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
               double bkgCoefficient, double dirichletCoefficient);
    virtual ~JMSmoother();
    double computeSmoothedProb(uint32_t dtf,uint32_t qtf);
private:

    uint32_t docTermCount_;
    uint32_t collectionTermCount_;
    double bkgCoefficient_;
    double dirichletCoefficient_;

};

#endif /* JMSMOOTHER_H_ */

/*
 * OkapiSmoother.h
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#ifndef OKAPISMOOTHER_H_
#define OKAPISMOOTHER_H_
typedef unsigned int uint32_t;
class OkapiSmoother {
public:
	OkapiSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
			 double bkgCoefficient, double dirichletCoefficient,
			 uint32_t docNum,double bm25k1, double bm25b);
	virtual ~OkapiSmoother();
	double computeSmoothedProb(uint32_t dtf,uint32_t qtf, uint32_t df);
private:

	uint32_t docTermCount_;
	uint32_t collectionTermCount_;
	double bkgCoefficient_;
	double dirichletCoefficient_;
	uint32_t docNum_;
	double bm25k1_, bm25b_;
};

#endif /* OKAPISMOOTHER_H_ */

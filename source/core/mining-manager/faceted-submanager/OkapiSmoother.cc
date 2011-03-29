/*
 * OkapiSmoother.cpp
 *
 *  Created on: 2011-1-30
 *      Author: yode
 */

#include "OkapiSmoother.h"
#include <math.h>
OkapiSmoother::OkapiSmoother(uint32_t docTermCount ,uint32_t collectionTermCount,
		 double bkgCoefficient, double dirichletCoefficient
		 ,uint32_t docNum,double bm25k1, double bm25b):
	  docTermCount_(docTermCount),collectionTermCount_(collectionTermCount)
	 ,bkgCoefficient_(bkgCoefficient),dirichletCoefficient_(dirichletCoefficient)
	 ,docNum_(docNum),bm25k1_(bm25k1),bm25b_(bm25b){
  }

OkapiSmoother::~OkapiSmoother() {
	// TODO Auto-generated destructor stub
}

double OkapiSmoother::computeSmoothedProb(uint32_t dtf,uint32_t qtf, uint32_t df) {

	double curTermIDF=log((docNum_-df+0.5)/(df+0.5));
    double avgDocLength=collectionTermCount_*1.0/docNum_;
    double param1=bm25k1_*(1-bm25b_);
    double param2=bm25k1_*bm25b_;
    double curDocLengthRatio=param2*docTermCount_/avgDocLength;
	return dtf*curTermIDF/(param1+curDocLengthRatio+dtf);

}


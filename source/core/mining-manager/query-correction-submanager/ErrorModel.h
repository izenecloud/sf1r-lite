/**
 * @brief  Definition of class Error model, it is used to compute score of noisy chaneel error model.
 * @author Jinli Liu
 * @date Sep 15,2009
 * @version v1.0
 */
/**
 * @brief  Defination of noisy channel error model.it suppoted english language.
 * @author  Jinli Liu
 * @date Oct 15,2009
 * @version v3.0
 * @detail
 * Log
 *      --delete noisy channel error model of korean language
 */
#ifndef __ERRORMODEL_H_
#define __ERRORMODEL_H_
#include <iostream>
#include <fstream>
#include<boost/tokenizer.hpp>
#include <util/ustring/UString.h>
#include <am/3rdparty/rde_hash.h>
namespace sf1r{
    class element
    {
      public:
        int x;
        int y;
        int flag;
        element(int i,int j,int mark):x(i),y(j),flag(mark)
        {}
    };
    class ErrorModel {

    public:
	        ErrorModel(){}
	       
		     /**
	         * @brief this function is used to compute score of noisy channel error model, it suppoted english language.
	         * @param queryUstring  this is query with type of izenelib::util::UString     
	         * @param candidateUstring    this is candidate obatained from candidateGeneration() with type of izenelib::util::UString   
	         * @return the score obtained from noisy channel error model.
	         */
		      float getErrorModel(const izenelib::util::UString& queryUstring, const izenelib::util::UString& candidateUstring);
		    /**
	         * @brief this function is used to assign matrics from give plain text used for error model. It suppoted english language.
	         * @param matrixFile  plain text used to store matrics    
	         */
		      void assignMatrix(const char* matrixFile);

      private:
              float sub[26][26];                 
              float rev[26][26];
              float del[27][26];
              float add[27][26];
              float charX[26];
              float charXY[26][26];     
    };
    /**
     * @brief this function is used to compute the edit distance matrix to see what kind of operations happened when changing one word into another
     * @param queryUstring  this is query with type of izenelib::util::UString     
     * @param candidateUstring    this is candidate obatained from candidateGeneration() with type of izenelib::util::UString 
     * @param operationVector constains the operation infomation  
    */
    void getOperations(const izenelib::util::UString& queryUstring, const izenelib::util::UString& candidateUstring, std::vector<element> & operations);
}
#endif/*__ERRORMODEL_H_*/

///
/// @file NERRanking.hpp
/// @brief Used for rank NE list
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-05-06
/// @date Updated 2010-05-06
///

#ifndef _NERRANKING_HPP_
#define _NERRANKING_HPP_
#include "TgTypes.h"
#include <util/ustring/UString.h>
namespace sf1r
{
    class NERRanking
    {
        public:
        static double getScore(const izenelib::util::UString& neName, uint32_t partialDF, uint32_t globalDF
        , uint32_t totalDocCount)
        {
            if( neName.length() <= 3 && partialDF<3 ) return 0.0;
//            for(size_t i=0;i<neName.length();i++)
//            {
//                if(neName.isChineseChar(i))
//                    return 0.0;
//            }
            double score = 0.0;
//            float inc = totalDocCount;
//            inc = inc/10000;
//            if(inc<100) inc = 2;
//
//            score = log(partialDF+1)/log((globalDF+inc)/log(sqrt(totalDocCount+1)+1)+1);
//            score=log(partialDF+1)/sqrt(globalDF+10);
            score = log(partialDF+1)/log(sqrt(globalDF)+3);
//            score=partialDF*(log(globalDF)+1)/(sqrt(globalDF+inc)+partialDF);
            if(neName.isChineseChar(0))
                score *= log(neName.length()+10);
            if(neName.length()>0&&neName.isChineseChar(0))
            {
                score*=2;
            }
            if(neName.length()==2&&neName.isChineseChar(0))
            {
                score*=0.2;
            }

            return score;
        }
        
    };
    
    
}

#endif 

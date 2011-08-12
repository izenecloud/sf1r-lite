/**
 * @file DistributeResultType.h
 * @author Zhongxia Li
 * @date Aug 12, 2011
 * @brief 
 */
#ifndef DISTIBUTE_RESULT_TYPE_H_
#define DISTIBUTE_RESULT_TYPE_H_

#include "ResultType.h"

//#include "type_defs.h"

namespace sf1r{

class KeywordPreSearchResult
{
    /// @brief document frequency info of terms for each property
    DocumentFrequencyInProperties dfmap_;

    /// @brief collection term frequency info of terms for each property
    CollectionTermFrequencyInProperties ctfmap_;

    MSGPACK_DEFINE();
};

class KeywordRealSearchResult
{

};

}

#endif /* DISTIBUTE_RESULT_TYPE_H_ */

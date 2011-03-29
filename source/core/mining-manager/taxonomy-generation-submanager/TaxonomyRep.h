///
/// @file TaxonomyRep.h
/// @brief The representation class for taxonomy result.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-05-18
/// @date Updated 2009-11-21 01:03:22
///

#ifndef TAXONOMYREP_H_
#define TAXONOMYREP_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include "TgTypes.h"
#include <common/type_defs.h>
#include <common/ResultType.h>
/**
* The memory representation form of a taxonomy. This information is used by presentation manager
* and the UI to display along with the ranking result.
* The APIs need to be specified.
*/

namespace sf1r{


/// @brief The memory representation form of a taxonomy.
class TaxonomyRep {
public:
    TaxonomyRep();
    virtual ~TaxonomyRep();
    TaxonomyRep(const TaxonomyRep& rhs): result_(rhs.result_), error_(rhs.error_)
    {
    }
    
    TaxonomyRep& operator=(const TaxonomyRep& rhs)
    {
        result_ = rhs.result_;
        error_ = rhs.error_;
        return *this;
    }
    
    std::vector<boost::shared_ptr<TgClusterRep> > result_;
    
    std::string error_;
    /** 
    * @brief To fill the mia result in sf1.
    * @param miaResult The mia result.
    */
    void fill(KeywordSearchResult& searchResult);
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & result_ & error_;
    }
    
private:
    
    /** 
    * @brief Recursive calling function to fill().
    * @param miaResult The mia result.
    * @param cluster The source to fill.
    * @param level The level in that tree.
    */
    static void fill_(KeywordSearchResult& searchResult, boost::shared_ptr<TgClusterRep> cluster, uint8_t level);
};
}
#endif /* TAXONOMYREP_H_ */

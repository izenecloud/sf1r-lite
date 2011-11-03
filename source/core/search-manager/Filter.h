/**
 * @file    Filter.h
 * @author  Yingfeng Zhang
 * @brief  Query filter
 */
#ifndef FILTER_H
#define FILTER_H

#include <ir/index_manager/utility/Ewah.h>

#include <vector>

#include <boost/shared_ptr.hpp>

using namespace izenelib::ir::indexmanager;

namespace sf1r{

/**
 * @brief Filter is nothing more than a bitmap that indicates whether a document
 * should be masked or not.  The bitmap is ususally generated through data from
 * BTreeIndex in Indexmanager. 
 */
class Filter
{
public:
    Filter(boost::shared_ptr<EWAHBoolArray<uword32> > docIdSet){
        uncompressedIDSet_.reset(new BoolArray<uword32>());
        docIdSet->toBoolArray(*uncompressedIDSet_);
    }
    virtual ~Filter(){}
public:
    bool test(docid_t docId){ return uncompressedIDSet_->get(docId); }
private:
    boost::shared_ptr<BoolArray<uword32> > uncompressedIDSet_;
};

}

#endif

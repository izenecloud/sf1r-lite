/**
 * @file    Filter.h
 * @author  Yingfeng Zhang
 * @brief  Query filter
 */
#ifndef FILTER_H
#define FILTER_H

#include <am/bitmap/Ewah.h>

#include <vector>

#include <boost/shared_ptr.hpp>

using namespace izenelib::am;

namespace sf1r{

/**
 * @brief Filter is nothing more than a bitmap that indicates whether a document
 * should be masked or not.  The bitmap is ususally generated through data from
 * BTreeIndex in Indexmanager. 
 */
class Filter
{
public:
    Filter(boost::shared_ptr<EWAHBoolArray<uint32_t> > docIdSet){
        uncompressedIDSet_.reset(new BoolArray<uint32_t>());
        docIdSet->toBoolArray(*uncompressedIDSet_);
    }
    virtual ~Filter(){}
public:
    bool test(docid_t docId){ return uncompressedIDSet_->get(docId); }
private:
    boost::shared_ptr<BoolArray<uint32_t> > uncompressedIDSet_;
};

}

#endif

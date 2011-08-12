/**
 * \file BTreeTermDocIterator.cpp
 * \brief 
 * \date Aug 9, 2011
 * \author Xin Liu
 */

#include "BTreeTermDocIterator.h"

#include <common/TermTypeDetector.h>
#include <index-manager/IndexManager.h>

#include <ir/index_manager/utility/BitMapIterator.h>
#include <ir/index_manager/utility/BitVector.h>

#include <boost/assert.hpp>

using namespace izenelib::ir::indexmanager;

namespace sf1r {

BTreeTermDocIterator::BTreeTermDocIterator(
        termid_t termId,
        const std::string& rawTerm,
        collectionid_t colID,
        IndexReader* pIndexReader,
        const std::string& property,
        unsigned int propertyId,
        sf1r::PropertyDataType propertyDataType,
        boost::shared_ptr<IndexManager> indexManagerPtr)
                :termId_(termId)
                ,rawTerm_(rawTerm)
                ,colID_(colID)
                ,pIndexReader_(pIndexReader)
                ,property_(property)
                ,propertyId_(propertyId)
                ,propertyDataType_(propertyDataType)
                ,pBTreeTermDocReader_(0)
                ,indexManagerPtr_(indexManagerPtr)
                ,df_(0)
                ,destroy_(true)
{
}

BTreeTermDocIterator::~BTreeTermDocIterator()
{
    if(destroy_)
         if(pBTreeTermDocReader_)
             delete pBTreeTermDocReader_;
}

bool BTreeTermDocIterator::accept()
{
    std::cout<<"*************enter the BTreeTermDocIterator::accept function************"<<std::endl;
    std::cout<<"***********rawTerm, property***********"<<rawTerm_<<", "<<property_<<std::endl;

    boost::shared_ptr<EWAHBoolArray<uword32> > pDocIdSet;
    boost::shared_ptr<BitVector> pBitVector;
    PropertyValue propertyValue;
    switch (propertyDataType_)
    {
        case UNSIGNED_INT_PROPERTY_TYPE:
            propertyValue = boost::lexical_cast< uint64_t >( rawTerm_ );
            break;
        case INT_PROPERTY_TYPE:
            propertyValue = boost::lexical_cast< int64_t >( rawTerm_ );
            break;
        case FLOAT_PROPERTY_TYPE:
            propertyValue = boost::lexical_cast< float >( rawTerm_ );
            break;
        default:
            break;
    }

    PropertyType value;
    PropertyValue2IndexPropertyType converter(value);
    boost::apply_visitor(converter, propertyValue.getVariant());
    pDocIdSet.reset(new EWAHBoolArray<uword32>());
    pBitVector.reset(new BitVector(pIndexReader_->numDocs() + 1));
    indexManagerPtr_->getDocsByPropertyValue(colID_, property_, value, *pBitVector);
    if (pBitVector->any())
    {
        pBitVector->compressed(*pDocIdSet);
        vector<uint> idList;
        pDocIdSet->appendRowIDs(idList);
        pBTreeTermDocReader_ = new BitMapIterator(idList);
        df_ = pBTreeTermDocReader_->docFreq();
        return true;
    }

    return false;
}

}

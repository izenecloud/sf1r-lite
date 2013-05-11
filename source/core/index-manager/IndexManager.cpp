#include "IndexManager.h"

#include <ir/index_manager/utility/StringUtils.h>
#include <ir/index_manager/utility/BitVector.h>

#include <common/Utilities.h>

#include <boost/assert.hpp>

#include <stdexcept>

namespace sf1r
{
std::string IndexManager::DATE("DATE");
IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

void IndexManager::convertData(const std::string& property, const PropertyValue& in, PropertyType& out)
{
    PropertyValue2IndexPropertyType converter(out);

    if (property == DATE)
    {
        int64_t time = in.get<int64_t>();
        PropertyValue inValue(time);
        boost::apply_visitor(converter, inValue.getVariant());
    }
    else
        boost::apply_visitor(converter, in.getVariant());
}

void IndexManager::makeRangeQuery(QueryFiltering::FilteringOperation filterOperation, const std::string& property,
        const std::vector<PropertyValue>& filterParam, boost::shared_ptr<FilterBitmapT> filterBitMap)
{
    collectionid_t colId = 1;
    std::string propertyL = boost::to_upper_copy(property);
    boost::scoped_ptr<BitVector> pBitVector;
    if(QueryFiltering::EQUAL != filterOperation) pBitVector.reset(new BitVector(pIndexReader_->maxDoc() + 1));

    switch (filterOperation)
    {
    case QueryFiltering::EQUAL:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL,filterParam[0], value);
        getDocsByPropertyValue(colId, property, value, *filterBitMap);
        return;
    }
    case QueryFiltering::INCLUDE:
    {
        std::vector<PropertyType> values(filterParam.size());
        for(std::size_t i = 0; i < filterParam.size(); ++i)
        {
            convertData(propertyL, filterParam[i], values[i]);
        }
        getDocsByPropertyValueIn(colId, property, values, *pBitVector, *filterBitMap);
        return;
    }
    case QueryFiltering::EXCLUDE:
    {
        std::vector<PropertyType> values(filterParam.size());
        for(std::size_t i = 0; i < filterParam.size(); ++i)
        {
            convertData(propertyL, filterParam[i], values[i]);
        }
        getDocsByPropertyValueNotIn(colId, property, values, *pBitVector);
    }
        break;
    case QueryFiltering::NOT_EQUAL:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueNotEqual(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::GREATER_THAN:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueGreaterThan(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::GREATER_THAN_EQUAL:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueGreaterThanOrEqual(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::LESS_THAN:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueLessThan(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::LESS_THAN_EQUAL:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueLessThanOrEqual(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::RANGE:
    {
        vector<docid_t> docs;
        PropertyType value1,value2;
        BOOST_ASSERT(filterParam.size() >= 2);
        convertData(propertyL, filterParam[0], value1);
        convertData(propertyL, filterParam[1], value2);

        getDocsByPropertyValueRange(colId, property, value1, value2, *pBitVector);
    }
        break;
    case QueryFiltering::PREFIX:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueStart(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::SUFFIX:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueEnd(colId, property, value, *pBitVector);
    }
        break;
    case QueryFiltering::SUB_STRING:
    {
        PropertyType value;
        BOOST_ASSERT(!filterParam.empty());
        convertData(propertyL, filterParam[0], value);
        getDocsByPropertyValueSubString(colId, property, value, *pBitVector);
    }
        break;
    default:
        break;
    }
    //Compress bit vector
    BOOST_ASSERT(pBitVector);
    pBitVector->compressed(*filterBitMap);
}

}

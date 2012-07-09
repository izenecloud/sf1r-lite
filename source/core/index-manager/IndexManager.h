/**
 * @file index-manager/IndexManager.h
 * @author Yingfeng Zhang
 * @brief Providing an encapsulation on operations that are not suitable to
 * put into izenelib for indexer
 */
#ifndef SF1V5_INDEX_MANAGER_H
#define SF1V5_INDEX_MANAGER_H

#include <common/type_defs.h>
#include <common/SFLogger.h>
#include <common/NumericPropertyTable.h>
#include <query-manager/ActionItem.h>

#include <ir/index_manager/index/Indexer.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/rtype/BTreeIndexerManager.h>

#include <util/string/StringUtils.h>

#include <boost/shared_ptr.hpp>

using namespace std;
using namespace izenelib::ir::indexmanager;

namespace sf1r
{

struct PropertyValue2IndexPropertyType
: public boost::static_visitor<>
{

    PropertyValue2IndexPropertyType(PropertyType& out)
    : out_(out)
    {
    }

    template<typename T>
    void operator()(const T& value)
    {
        throw std::runtime_error("Type not supported in PropertyType");
    }
    void operator()(int32_t value)
    {
        out_ = static_cast<int32_t>(value);
    }
    void operator()(int64_t value)
    {
        out_ = static_cast<int64_t>(value);
    }
    void operator()(float value)
    {
        out_ = value;
    }
    void operator()(double value)
    {
        out_ = value;
    }
    void operator()(const std::string& value)
    {
        izenelib::util::Trim(const_cast<std::string&>(value));
        out_ = izenelib::util::UString(value,izenelib::util::UString::UTF_8);
    }
    void operator()(const izenelib::util::UString& value)
    {
        izenelib::util::Trim(const_cast<izenelib::util::UString&>(value));
        out_ = value;
    }

private:
    PropertyType& out_;
};


template<typename T>
struct NumericUtil
{
static T Low() {return (T)-0x80000000;}
static T High() {return (T)0x7FFFFFFF;}
};

template<>
struct NumericUtil<unsigned int>
{
static unsigned int Low() {return 0;}
static unsigned int High() {return 0xFFFFFFFF;}
};

template<>
struct NumericUtil<uint64_t>
{
static uint64_t Low() {return 0LLU;}
static uint64_t High() {return 0xFFFFFFFFFFFFFFFFLLU;}
};

template<>
struct NumericUtil<int64_t>
{
static uint64_t Low() {return -0x8000000000000000LL;}
static uint64_t High() {return 0x7FFFFFFFFFFFFFFFLL;}
};


class SearchManager;
class IndexManager: public izenelib::ir::indexmanager::Indexer
{
friend class QueryBuilder;
public:
    IndexManager();

    ~IndexManager();

public:
    ///load data for BTree index.
    ///@param data - the buffer provided by user
    template<typename T>
    bool loadPropertyDataForSorting(const string& property, PropertyDataType type, boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable)
    {
        BTreeIndexer<T>* pBTreeIndexer = pBTreeIndexer_->getIndexer<T>(property);
        std::size_t size = getIndexReader()->maxDoc();
        if (!size)
        {
            return false;
        }
        size += 1;
        if (!numericPropertyTable)
            numericPropertyTable.reset(new NumericPropertyTable<T>(type));
        numericPropertyTable->resize(size);

        NumericPropertyTableBase::WriteLock lock(numericPropertyTable->mutex_);
        T* data = (T *)numericPropertyTable->getValueList();
        T low = NumericUtil<T>::Low();
        T high = NumericUtil<T>::High();
        return pBTreeIndexer->getValueBetween(low, high, size, data);
    }

    bool convertStringPropertyDataForSorting(const string& property, boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable);

    ///Make range query on BTree index to fill the Filter, which is required by the filter utility of SearchManager
    void makeRangeQuery(QueryFiltering::FilteringOperation filterOperation, const std::string& property,
           const std::vector<PropertyValue>& filterParam, boost::shared_ptr<BitVector> docIdSet);

private:
    static void convertData(const std::string& property, const PropertyValue& in, PropertyType& out);
private:
    static std::string DATE;
};

}

#endif

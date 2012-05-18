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
    void operator()(int64_t value)
    {
        out_ = static_cast<int64_t>(value);
    }
    void operator()(uint64_t value)
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
static T Low() {return (T)-0x7FFFFFFF;}
static T High() {return (T)0x7FFFFFFF;}
};

template<>
struct NumericUtil<unsigned int>
{
static unsigned int Low() {return 0;}
static unsigned int High() {return 0xFFFFFFFF;}
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
    void loadPropertyDataForSorting(const string& property, T* &data, size_t&size)
    {
        BTreeIndexer<T>* pBTreeIndexer = pBTreeIndexer_->getIndexer<T>(property);
        size = getIndexReader()->maxDoc();
        if(!size)
        {
            data = NULL;
            return;
        }
        size += 1;
        T low = NumericUtil<T>::Low();
        T high = NumericUtil<T>::High();
        data = new T[size]();
        memset(data,0,size * sizeof(T));
        if (pBTreeIndexer->getValueBetween(low,high,size,data) == 0)
        {
            delete[] data;
            data = NULL;
        }
    }

    void convertStringPropertyDataForSorting(const string& property, uint32_t* &data, size_t&size);

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

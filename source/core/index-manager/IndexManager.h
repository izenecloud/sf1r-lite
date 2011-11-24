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
#include <ir/index_manager/utility/BitVector.h>
#include <ir/index_manager/index/rtype/BTreeIndexerManager.h>
#include <boost/shared_ptr.hpp>

#define MAX_NUMERICSIZER 32768

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
        izenelib::ir::indexmanager::trim(const_cast<std::string&>(value));
        out_ = izenelib::util::UString(value,izenelib::util::UString::UTF_8);
    }
    void operator()(const izenelib::util::UString& value)
    {
        izenelib::ir::indexmanager::trim(const_cast<izenelib::util::UString&>(value));
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
    void loadPropertyDataForSorting(const string& property, T* &data)
    {
//         int32_t fid = getPropertyIDByName(1,property);
//         collectionid_t cid = 1;
//         size_t maxLength = getIndexReader()->maxDoc()+1;
// 
//         BTreeIndex<IndexKeyType<T> >* pBTreeIndexer = pBTreeIndexer_->getIndexer<T>();
//         T low = NumericUtil<T>::Low();
//         T high = NumericUtil<T>::High();
//         data = new T[maxLength]();
//         IndexKeyType<T> lowkey(cid, fid, low);
//         IndexKeyType<T> highkey(cid, fid, high);
//         if (pBTreeIndexer->get_between(lowkey,highkey,data,maxLength) == 0)
//         {
//             delete[] data;
//             data = NULL;
//         }
        CBTreeIndexer<T>* pBTreeIndexer = pBTreeIndexer_->getIndexer<T>(property);
        size_t maxLength = getIndexReader()->maxDoc()+1;
        T low = NumericUtil<T>::Low();
        T high = NumericUtil<T>::High();
        data = new T[maxLength]();
        if (pBTreeIndexer->getValueBetween(low,high,maxLength, data) == 0)
        {
            delete[] data;
            data = NULL;
        }
    }

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

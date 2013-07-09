/**
 * @file index-manager/IndexManager.h
 * @author Yingfeng Zhang
 * @brief Providing an encapsulation on operations that are not suitable to
 * put into izenelib for indexer
 */
#ifndef SF1V5_INVERTED_INDEX_MANAGER_H
#define SF1V5_INVERTED_INDEX_MANAGER_H

#include <common/type_defs.h>
#include <common/SFLogger.h>
#include <query-manager/ActionItem.h>

#include <ir/index_manager/index/Indexer.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/rtype/BTreeIndexerManager.h>
#include <ir/index_manager/utility/EWAHTermDocFreqs.h>
#include <ir/id_manager/IDManager.h>

#include <util/string/StringUtils.h>
#include <configuration-manager/ConfigurationTool.h>
#include <configuration-manager/PropertyConfig.h>
#include <document-manager/Document.h>

#include <3rdparty/am/stx/btree_map.h>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "IIncSupportedIndex.h"

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

class LAManager;
class SearchManager;
class IndexModeSelector;
class IndexBundleConfiguration;
class DocumentManager;

class InvertedIndexManager: public izenelib::ir::indexmanager::Indexer,
    public IIncSupportedIndex, public boost::enable_shared_from_this<InvertedIndexManager>
{
friend class QueryBuilder;
public:
    InvertedIndexManager(IndexBundleConfiguration* bundleConfig);

    ~InvertedIndexManager();

public:
    typedef uint64_t FilterWordT;
    typedef EWAHBoolArray<FilterWordT> FilterBitmapT;
    typedef EWAHTermDocFreqs<FilterWordT> FilterTermDocFreqsT;

    ///Make range query on BTree index to fill the Filter, which is required by the filter utility of SearchManager
    void makeRangeQuery(QueryFiltering::FilteringOperation filterOperation, const std::string& property,
           const std::vector<PropertyValue>& filterParam, boost::shared_ptr<FilterBitmapT> filterBitMap);

    virtual bool isRealTime();
    virtual void flush(bool force = true);
    virtual void optimize(bool wait);
    virtual void preBuildFromSCD(size_t total_filesize);
    virtual void postBuildFromSCD(time_t timestamp);
    virtual void preMining();
    virtual void postMining();
    virtual void finishIndex();
    virtual void finishRebuild();
    virtual void preProcessForAPI();
    virtual void postProcessForAPI();

    virtual bool insertDocument(const Document& doc, time_t timestamp);
    virtual bool updateDocument(const Document& olddoc, const Document& old_rtype_doc, const Document& newdoc, int updateType, time_t timestamp);
    virtual void removeDocument(docid_t docid, time_t timestamp);

private:
    static void convertData(const std::string& property, const PropertyValue& in, PropertyType& out);
    bool makeForwardIndex_(
            docid_t docId,
            const izenelib::util::UString& text,
            const std::string& propertyName,
            unsigned int propertyId,
            const AnalysisInfo& analysisInfo,
            boost::shared_ptr<LAInput>& laInput);

    bool checkSeparatorType_(
            const Document::doc_prop_value_strtype& propertyValueStr,
            izenelib::util::UString::EncodingType encoding,
            char separator);

    void prepareIndexDocumentCommon(const Document& newdoc,
        const IndexBundleSchema& schema, izenelib::ir::indexmanager::IndexerDocument& indexdoc);
    bool prepareIndexDocumentForInsert(const Document& newdoc, const IndexBundleSchema& schema, izenelib::ir::indexmanager::IndexerDocument& indexdoc);
    bool prepareIndexDocumentForUpdate(const Document& olddoc, const Document& old_rtype_doc, const Document& newdoc,
        int updateType, const IndexBundleSchema& schema, 
        izenelib::ir::indexmanager::IndexerDocument& new_indexdoc, izenelib::ir::indexmanager::IndexerDocument& old_indexdoc);

    bool prepareIndexDocumentStringProperty_(
            docid_t docId,
            const std::string& property_name,
            const izenelib::util::UString& propertyValueU,
            IndexBundleSchema::const_iterator iter,
            izenelib::ir::indexmanager::IndexerDocument& indexDocument);

    bool prepareIndexRTypeProperties_(
            docid_t docId, const IndexBundleSchema& schema,
            izenelib::ir::indexmanager::IndexerDocument& indexDocument);
    bool prepareIndexRTypeProperties_(
            docid_t docId, const Document& old_rtype_doc,
            const IndexBundleSchema& schema,
            izenelib::ir::indexmanager::IndexerDocument& indexDocument);

    bool prepareIndexDocumentNumericProperty_(
            docid_t docId,
            const Document::doc_prop_value_strtype& propertyValueU,
            IndexBundleSchema::const_iterator iter,
            izenelib::ir::indexmanager::IndexerDocument& indexDocument);

    bool mergeDocument_(
        const Document& olddoc,
        const Document& newdoc,
        IndexerDocument& indexDocument);

private:
    // update type, newdoc, olddoc, newindexdoc, oldindexdoc.
    boost::shared_ptr<IndexModeSelector> index_mode_selector_;
    unsigned int collectionId_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;
    IndexBundleConfiguration* bundleConfig_;
    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;
    PropertyConfig dateProperty_;

    friend class IndexBundleActivator;
    //friend class ProductBundleActivator;
};

}

#endif

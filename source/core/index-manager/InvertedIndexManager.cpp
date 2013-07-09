#include "InvertedIndexManager.h"
#include "IndexModeSelector.h"

#include <la-manager/LAManager.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <configuration-manager/PropertyConfig.h>
#include <document-manager/DocumentManager.h>
#include <ir/index_manager/utility/StringUtils.h>
#include <ir/index_manager/utility/BitVector.h>
#include <common/NumericPropertyTable.h>
#include <common/NumericRangePropertyTable.h>
#include <common/RTypeStringPropTable.h>

#include <common/Utilities.h>

#include <boost/assert.hpp>

#include <stdexcept>

namespace
{
const static std::string DOCID("DOCID");
const static std::string DATE("DATE");
//sf1r::PropertyConfig tempPropertyConfig;

enum UpdateType
{
    UNKNOWN,
    INSERT, ///Not update, it's a new document
    GENERAL, ///General update, equals to del + insert
    REPLACE, ///Don't need to change index, just adjust DocumentManager
    RTYPE  ///RType update to index
};

void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator)
{
    izenelib::util::UString str(szText);
    izenelib::util::UString sep(" ", encoding);
    sep[0] = Separator;
    size_t n = 0, nOld = 0;
    while (n != izenelib::util::UString::npos)
    {
        n = str.find(sep,n);
        if (n != izenelib::util::UString::npos)
        {
            if (n != nOld)
                out.push_back(sf1r::ustr_to_propstr(str.substr(nOld, n - nOld)));
            n += sep.length();
            nOld = n;
        }
    }
    out.push_back(sf1r::ustr_to_propstr(str.substr(nOld)));
}

void split_int32(const std::string& str, std::list<PropertyType>& out, const char* sep)
{
    std::size_t n = 0, nOld = 0;
    while (n != std::string::npos)
    {
        n = str.find_first_of(sep, n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n - nOld);
                    boost::algorithm::trim(tmpStr);
                    int32_t value = boost::lexical_cast<int32_t>(tmpStr);
                    out.push_back(value);
                }
                catch (boost::bad_lexical_cast &)
                {
                }
            }
            ++n;
            nOld = n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld);
        boost::algorithm::trim(tmpStr);
        int32_t value = boost::lexical_cast<int32_t>(tmpStr);
        out.push_back(value);
    }
    catch (boost::bad_lexical_cast &)
    {
    }
}

void split_int64(const std::string& str, std::list<PropertyType>& out, const char* sep)
{
    std::size_t n = 0, nOld = 0;
    while (n != std::string::npos)
    {
        n = str.find_first_of(sep, n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n - nOld);
                    boost::algorithm::trim(tmpStr);
                    int64_t value = boost::lexical_cast<int64_t>(tmpStr);
                    out.push_back(value);
                }
                catch (boost::bad_lexical_cast &)
                {
                }
            }
            ++n;
            nOld = n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld);
        boost::algorithm::trim(tmpStr);
        int64_t value = boost::lexical_cast<int64_t>(tmpStr);
        out.push_back(value);
    }
    catch (boost::bad_lexical_cast &)
    {
    }
}

void split_float(const std::string& str, std::list<PropertyType>& out, const char* sep)
{
    std::size_t n = 0, nOld = 0;
    while (n != std::string::npos)
    {
        n = str.find_first_of(sep, n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                try
                {
                    std::string tmpStr = str.substr(nOld, n - nOld);
                    boost::algorithm::trim(tmpStr);
                    float value = boost::lexical_cast<float>(tmpStr);
                    out.push_back(value);
                }
                catch (boost::bad_lexical_cast &)
                {
                }
            }
            ++n;
            nOld = n;
        }
    }

    try
    {
        std::string tmpStr = str.substr(nOld);
        boost::algorithm::trim(tmpStr);
        float value = boost::lexical_cast<float>(tmpStr);
        out.push_back(value);
    }
    catch (boost::bad_lexical_cast &)
    {
    }
}

void split_datetime(const std::string& str, std::list<PropertyType>& out, const char* sep)
{
    std::size_t n = 0, nOld = 0;
    while (n != std::string::npos)
    {
        n = str.find_first_of(sep,n);
        if (n != std::string::npos)
        {
            if (n != nOld)
            {
                std::string tmpStr = str.substr(nOld, n-nOld);
                boost::algorithm::trim(tmpStr);
                time_t timestamp = sf1r::Utilities::createTimeStampInSeconds(tmpStr);
                out.push_back(timestamp);
            }
            ++n;
            nOld = n;
        }
    }

    std::string tmpStr = str.substr(nOld);
    boost::algorithm::trim( tmpStr );
    time_t timestamp = sf1r::Utilities::createTimeStampInSeconds(tmpStr);
    out.push_back(timestamp);
}


}

using izenelib::ir::idmanager::IDManager;

namespace sf1r
{

InvertedIndexManager::InvertedIndexManager(IndexBundleConfiguration* bundleConfig)
    :bundleConfig_(bundleConfig)
{
    bool hasDateInConfig = false;
    collectionId_ = 1;
    const IndexBundleSchema& indexSchema = bundleConfig_->indexSchema_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
            iter != iterEnd; ++iter)
    {
        std::string propertyName = iter->getName();
        boost::to_lower(propertyName);
        if (propertyName == "date")
        {
            hasDateInConfig = true;
            dateProperty_ = *iter;
            break;
        }
    }
    if (!hasDateInConfig)
        throw std::runtime_error("Date Property Doesn't exist in config");

    config_tool::buildPropertyAliasMap(bundleConfig_->indexSchema_, propertyAliasMap_);
}

InvertedIndexManager::~InvertedIndexManager()
{
}

void InvertedIndexManager::convertData(const std::string& property, const PropertyValue& in, PropertyType& out)
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

void InvertedIndexManager::makeRangeQuery(QueryFiltering::FilteringOperation filterOperation, const std::string& property,
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

void InvertedIndexManager::flush(bool force)
{
    izenelib::ir::indexmanager::Indexer::flush(force);
}

void InvertedIndexManager::optimize(bool wait)
{
    izenelib::ir::indexmanager::Indexer::optimizeIndex();
    if (wait)
        waitForMergeFinish();
}

bool InvertedIndexManager::isRealTime()
{
    return izenelib::ir::indexmanager::Indexer::isRealTime();
}

void InvertedIndexManager::preBuildFromSCD(size_t total_filesize)
{
    //here, try to set the index mode(default[batch] or realtime)
    //The threshold is set to the scd_file_size/exist_doc_num, if smaller or equal than this threshold then realtime mode will turn on.
    //when the scd file size(M) larger than max_realtime_msize, the default mode will turn on while ignore the threshold above.
    long max_realtime_msize = 50L; //set to 50M
    double threshold = 50.0/500000.0;
    index_mode_selector_.reset(new IndexModeSelector(shared_from_this(), threshold, max_realtime_msize));
    index_mode_selector_->TrySetIndexMode(total_filesize);

    if (!isRealTime())
    {
        setIndexMode("realtime");
        flush();
        deletebinlog();
        setIndexMode("default");
    }
}

void InvertedIndexManager::postBuildFromSCD(time_t timestamp)
{
    if (index_mode_selector_)
        index_mode_selector_->TryCommit();
}

void InvertedIndexManager::preMining()
{
    pauseMerge();
}

void InvertedIndexManager::postMining()
{
    resumeMerge();
}

void InvertedIndexManager::finishIndex()
{
    getIndexReader();
    index_mode_selector_.reset();
}

void InvertedIndexManager::finishRebuild()
{
    flush();
}

void InvertedIndexManager::preProcessForAPI()
{
    if (!isRealTime())
    	setIndexMode("realtime");
}

void InvertedIndexManager::postProcessForAPI()
{
}

bool InvertedIndexManager::prepareIndexRTypeProperties_(
        docid_t docId,
        const Document& old_rtype_doc,
        const IndexBundleSchema& schema,
        IndexerDocument& indexDocument)
{
    sf1r::PropertyConfig tempPropertyConfig;
    IndexerPropertyConfig indexerPropertyConfig;
    Document::property_const_iterator it = old_rtype_doc.propertyBegin();
    for (; it != old_rtype_doc.propertyEnd(); ++it)
    {
        tempPropertyConfig.propertyName_ = it->first;
        IndexBundleSchema::const_iterator index_it = schema.find(tempPropertyConfig);
        if (index_it == schema.end())
            continue;
        if (index_it->getType() == STRING_PROPERTY_TYPE ||
            index_it->getType() == SUBDOC_PROPERTY_TYPE)
        {
            prepareIndexDocumentStringProperty_(docId, it->first, 
                it->second.getPropertyStrValue(), index_it, indexDocument);
        }
        else
        {
            prepareIndexDocumentNumericProperty_(docId, it->second.getPropertyStrValue(),
                index_it, indexDocument);
        }
    }
    return true;
}

bool InvertedIndexManager::prepareIndexRTypeProperties_(
        docid_t docId,
        const IndexBundleSchema& schema,
        IndexerDocument& indexDocument)
{
    sf1r::PropertyConfig tempPropertyConfig;
    IndexerPropertyConfig indexerPropertyConfig;
    DocumentManager::RTypeStringPropTableMap& rtype_string_proptable = documentManager_->getRTypeStringPropTableMap();
    for (DocumentManager::RTypeStringPropTableMap::const_iterator rtype_it = rtype_string_proptable.begin();
            rtype_it != rtype_string_proptable.end(); ++rtype_it)
    {
        tempPropertyConfig.propertyName_ = rtype_it->first;
        IndexBundleSchema::const_iterator index_it = schema.find(tempPropertyConfig);
        if (index_it == schema.end())
            continue;
        std::string s_propvalue;
        rtype_it->second->getRTypeString(docId, s_propvalue);
        prepareIndexDocumentStringProperty_(docId, rtype_it->first, 
            str_to_propstr(s_propvalue, bundleConfig_->encoding_), index_it, indexDocument);
    }

    const DocumentManager::NumericPropertyTableMap& numericPropertyTables = documentManager_->getNumericPropertyTableMap();
    bool ret = true;
    for (DocumentManager::NumericPropertyTableMap::const_iterator it = numericPropertyTables.begin();
            it != numericPropertyTables.end(); ++it)
    {
        tempPropertyConfig.propertyName_ = it->first;
        IndexBundleSchema::const_iterator iter = schema.find(tempPropertyConfig);

        if (iter == schema.end())
            continue;

        indexerPropertyConfig.setPropertyId(iter->getPropertyId());
        indexerPropertyConfig.setName(iter->getName());
        indexerPropertyConfig.setIsIndex(iter->isIndex());
        indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
        indexerPropertyConfig.setIsFilter(iter->getIsFilter());
        indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
        indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

        switch (iter->getType())
        {
        case INT32_PROPERTY_TYPE:
        case INT8_PROPERTY_TYPE:
        case INT16_PROPERTY_TYPE:
            if (iter->getIsRange())
            {
                std::pair<int32_t, int32_t> value;
                NumericRangePropertyTable<int32_t>* numericPropertyTable = static_cast<NumericRangePropertyTable<int32_t> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                {
                    ret = false;
                    break;
                }

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                int32_t value;
                if (!it->second->getInt32Value(docId, value))
                {
                    ret = false;
                    break;
                }
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            break;
        case FLOAT_PROPERTY_TYPE:
            if (iter->getIsRange())
            {
                std::pair<float, float> value;
                NumericRangePropertyTable<float>* numericPropertyTable = static_cast<NumericRangePropertyTable<float> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                {
                    ret = false;
                    break;
                }

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                float value;
                if (!it->second->getFloatValue(docId, value))
                {
                    ret = false;
                    break;
                }
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            break;
        case DATETIME_PROPERTY_TYPE:
        case INT64_PROPERTY_TYPE:
            if (iter->getIsRange())
            {
                std::pair<int64_t, int64_t> value;
                NumericRangePropertyTable<int64_t>* numericPropertyTable = static_cast<NumericRangePropertyTable<int64_t> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                {
                    ret = false;
                    break;
                }

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                int64_t value;
                if (!it->second->getInt64Value(docId, value))
                {
                    ret = false;
                    break;
                }
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            break;
        default:
            break;
        }
        if (!ret)
        {
            LOG(ERROR) << "get number property failed.";
        }
        ret = true;
    }
    return ret;
}

/// @desc Make a forward index of a given text.
/// You can specify an Language Analysis option through AnalysisInfo parameter.
/// You have to get a proper AnalysisInfo value from the configuration. (Currently not implemented.)
bool InvertedIndexManager::makeForwardIndex_(
        docid_t docId,
        const izenelib::util::UString& text,
        const std::string& propertyName,
        unsigned int propertyId,
        const AnalysisInfo& analysisInfo,
        boost::shared_ptr<LAInput>& laInput)
{
    laInput.reset(new LAInput);
    laInput->setDocId(docId);

    la::MultilangGranularity indexingLevel = bundleConfig_->indexMultilangGranularity_;
    if (indexingLevel == la::SENTENCE_LEVEL)
    {
        if (bundleConfig_->bIndexUnigramProperty_)
        {
            if (propertyName.find("_unigram") != std::string::npos)
                indexingLevel = la::FIELD_LEVEL;  /// for unigram property, we do not need sentence level indexing
        }
    }

    if (!laManager_->getTermIdList(idManager_.get(), text, analysisInfo, *laInput, indexingLevel))
        return false;
    return true;
}

bool InvertedIndexManager::prepareIndexDocumentStringProperty_(
        docid_t docId,
        const std::string& property_name,
        const IndexPropString& propertyValueU,
        IndexBundleSchema::const_iterator iter,
        IndexerDocument& indexDocument)
{
    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

    if (propertyValueU.empty())
        return true;

    ///process for properties that requires forward index to be created
    if (iter->isIndex())
    {
        const AnalysisInfo& analysisInfo = iter->getAnalysisInfo();
        if (analysisInfo.analyzerId_.empty())
        {
            if (iter->getIsFilter() && iter->getIsMultiValue())
            {
                MultiValuePropertyType props;
                split_string(propstr_to_ustr(propertyValueU), props, encoding, ',');
                indexDocument.insertProperty(indexerPropertyConfig, props);
            }
            else
                indexDocument.insertProperty(indexerPropertyConfig, propertyValueU);
        }
        else
        {
            boost::shared_ptr<LAInput> laInput;
            if (!makeForwardIndex_(docId, propertyValueU, property_name, iter->getPropertyId(), analysisInfo, laInput))
            {
                LOG(ERROR) << "Forward Indexing Failed Error";
                return false;
            }
            if (iter->getIsFilter())
            {
                if (iter->getIsMultiValue())
                {
                    MultiValuePropertyType props;
                    split_string(propstr_to_ustr(propertyValueU), props, encoding,',');

                    MultiValueIndexPropertyType indexData =
                        std::make_pair(laInput, props);
                    indexDocument.insertProperty(indexerPropertyConfig, indexData);
                }
                else
                {
                    IndexPropertyType indexData = std::make_pair(
                        laInput, propertyValueU);
                    indexDocument.insertProperty(indexerPropertyConfig, indexData);
                }
            }
            else
                indexDocument.insertProperty(
                    indexerPropertyConfig, laInput);

            // For alias indexing
            config_tool::PROPERTY_ALIAS_MAP_T::iterator mapIter =
                propertyAliasMap_.find(iter->getName());
            if (mapIter != propertyAliasMap_.end()) // if there's alias property
            {
                std::vector<PropertyConfig>::iterator vecIter =
                    mapIter->second.begin();
                for (; vecIter != mapIter->second.end(); vecIter++)
                {
                    AnalysisInfo aliasAnalysisInfo =
                        vecIter->getAnalysisInfo();
                    if (!makeForwardIndex_(
                            docId,
                            propertyValueU,
                            property_name,
                            vecIter->getPropertyId(),
                            aliasAnalysisInfo, laInput))
                    {
                        LOG(ERROR) << "Forward Indexing Failed";
                        return false;
                    }
                    IndexerPropertyConfig aliasIndexerPropertyConfig(
                        vecIter->getPropertyId(),
                        vecIter->getName(),
                        vecIter->isIndex(),
                        vecIter->isAnalyzed());
                    aliasIndexerPropertyConfig.setIsFilter(vecIter->getIsFilter());
                    aliasIndexerPropertyConfig.setIsMultiValue(vecIter->getIsMultiValue());
                    aliasIndexerPropertyConfig.setIsStoreDocLen(vecIter->getIsStoreDocLen());
                    indexDocument.insertProperty(
                        aliasIndexerPropertyConfig, laInput);
                } // end - for
            } // end - if (mapIter != end())
        }
    }
    else
    {
        //other extra properties that need not be in index manager
        indexDocument.insertProperty(indexerPropertyConfig, propertyValueU);
    }
    return true;
}

bool InvertedIndexManager::checkSeparatorType_(
        const Document::doc_prop_value_strtype& propertyValueStr,
        izenelib::util::UString::EncodingType encoding,
        char separator)
{
    izenelib::util::UString tmpStr(propstr_to_ustr(propertyValueStr));
    izenelib::util::UString sep(" ", encoding);
    sep[0] = separator;
    size_t n = 0;
    n = tmpStr.find(sep,0);
    if (n != izenelib::util::UString::npos)
        return true;
    return false;
}

bool InvertedIndexManager::prepareIndexDocumentNumericProperty_(
        docid_t docId,
        const Document::doc_prop_value_strtype& propertyValueU,
        IndexBundleSchema::iterator iter,
        IndexerDocument& indexDocument)
{
    if (!iter->isIndex()) return false;

    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;

    const std::string prop_str = propstr_to_str(propertyValueU, encoding);

    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

    switch (iter->getType())
    {
    case INT32_PROPERTY_TYPE:
    case INT8_PROPERTY_TYPE:
    case INT16_PROPERTY_TYPE:
    {
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_int32(prop_str, props, ",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            int32_t value = 0;
            try
            {
                value = boost::lexical_cast<int32_t>(prop_str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                {
                    split_int32(prop_str, multiProps, "-");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                {
                    split_int32(prop_str, multiProps, "~");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                {
                    split_int32(prop_str, multiProps, ",");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else
                {
                    try
                    {
                        value = (int32_t)(boost::lexical_cast<float>(prop_str));
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                    catch (const boost::bad_lexical_cast &)
                    {
                        //LOG(ERROR) << "Wrong format of number value. DocId " << docId <<" Property "<<fieldStr<< " Value" << prop_str;
                    }
                }
            }
        }
        break;
    }
    case FLOAT_PROPERTY_TYPE:
    {
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_float(prop_str, props, ",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            float value = 0;
            try
            {
                value = boost::lexical_cast<float>(prop_str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                    split_float(prop_str, multiProps, "-");
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                    split_float(prop_str, multiProps, "~");
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                    split_float(prop_str, multiProps, ",");
                indexerPropertyConfig.setIsMultiValue(true);
                indexDocument.insertProperty(indexerPropertyConfig, multiProps);
            }
        }
        break;
    }
    case DATETIME_PROPERTY_TYPE:
    {
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_datetime(prop_str, props, ",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            time_t timestamp = Utilities::createTimeStampInSeconds(propertyValueU);
            indexDocument.insertProperty(indexerPropertyConfig, timestamp);
        }
        break;
    }
    case INT64_PROPERTY_TYPE:
    {
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_int64(prop_str, props, ",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            int64_t value = 0;
            try
            {
                value = boost::lexical_cast<int64_t>(prop_str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                {
                    split_int64(prop_str, multiProps, "-");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                {
                    split_int64(prop_str, multiProps, "~");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                {
                    split_int64(prop_str, multiProps, ",");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else
                {
                    try
                    {
                        value = (int64_t)(boost::lexical_cast<float>(prop_str));
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                    catch (const boost::bad_lexical_cast &)
                    {
                        //LOG(ERROR) << "Wrong format of number value. DocId " << docId <<" Property "<<fieldStr<< " Value" << prop_str;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return true;
}

void InvertedIndexManager::prepareIndexDocumentCommon(const Document& document,
    const IndexBundleSchema& schema, IndexerDocument& indexdoc)
{
    docid_t docId = document.getId();
    Document::property_const_iterator it = document.propertyBegin();

    sf1r::PropertyConfig tempPropertyConfig;
    for (; it != document.propertyEnd(); ++it)
    {
        const std::string& propertyName = it->first;
        tempPropertyConfig.propertyName_ = propertyName;
        IndexBundleSchema::const_iterator iter = schema.find(tempPropertyConfig);
        bool isIndexSchema = (iter != schema.end());

        if (boost::iequals(propertyName, DOCID))
        {
            continue;
        }
        else if (boost::iequals(propertyName, dateProperty_.getName()))
        {
            continue;
        }
        else if (isIndexSchema)
        {
            const Document::doc_prop_value_strtype& propValue = document.property(it->first).getPropertyStrValue();
            switch(iter->getType())
            {
            case STRING_PROPERTY_TYPE:
            case SUBDOC_PROPERTY_TYPE:
                {
                    prepareIndexDocumentStringProperty_(docId, propertyName, propValue, iter, indexdoc);
                }
                break;
            case INT32_PROPERTY_TYPE:
            case FLOAT_PROPERTY_TYPE:
            case INT8_PROPERTY_TYPE:
            case INT16_PROPERTY_TYPE:
            case INT64_PROPERTY_TYPE:
            case DOUBLE_PROPERTY_TYPE:
            case NOMINAL_PROPERTY_TYPE:
            case DATETIME_PROPERTY_TYPE:
                if (iter->getIsFilter() && !iter->getIsMultiValue())
                {
                }
                else
                {
                    prepareIndexDocumentNumericProperty_(docId, propValue, iter, indexdoc);
                }
                break;
            default:
                break;
            }
        }
    }
}

bool InvertedIndexManager::prepareIndexDocumentForInsert(const Document& newdoc,
    const IndexBundleSchema& schema, IndexerDocument& indexdoc)
{
    docid_t docId = newdoc.getId();
    indexdoc.setOldId(0);
    indexdoc.setDocId(docId, collectionId_);
    prepareIndexDocumentCommon(newdoc, schema, indexdoc);

    return true;
}

bool InvertedIndexManager::prepareIndexDocumentForUpdate(const Document& olddoc, const Document& old_rtype_doc,
    const Document& newdoc, int updateType,
    const IndexBundleSchema& schema, IndexerDocument& new_indexdoc,
    IndexerDocument& old_indexdoc)
{
    docid_t docId = newdoc.getId();
    docid_t oldId = olddoc.getId();
    if (updateType == RTYPE)
    {
        prepareIndexRTypeProperties_(oldId, old_rtype_doc, schema, old_indexdoc);
    }
    new_indexdoc.setOldId(oldId);
    new_indexdoc.setDocId(docId, collectionId_);
    prepareIndexDocumentCommon(newdoc, schema, new_indexdoc);

    return true;
}

bool InvertedIndexManager::insertDocument(const Document& newdoc, time_t timestamp)
{
    IndexerDocument indexdoc;
    prepareIndexDocumentForInsert(newdoc, bundleConfig_->indexSchema_, indexdoc);

    prepareIndexRTypeProperties_(newdoc.getId(), bundleConfig_->indexSchema_, indexdoc);
    return izenelib::ir::indexmanager::Indexer::insertDocument(indexdoc);
}

bool InvertedIndexManager::updateDocument(const Document& olddoc, const Document& old_rtype_doc, const Document& newdoc,
    int updateType, time_t timestamp)
{
    IndexerDocument old_indexdoc;
    IndexerDocument new_indexdoc;
    prepareIndexDocumentForUpdate(olddoc, old_rtype_doc, newdoc, updateType, bundleConfig_->indexSchema_,
        new_indexdoc, old_indexdoc);

    prepareIndexRTypeProperties_(newdoc.getId(), bundleConfig_->indexSchema_, new_indexdoc);

    switch (updateType)
    {
    case GENERAL:
    {
        //uint32_t oldId = new_indexdoc.getOldId();
        if( mergeDocument_(olddoc, newdoc, new_indexdoc) )
        {
            izenelib::ir::indexmanager::Indexer::updateDocument(new_indexdoc);
        }
        break;
    }
    case RTYPE:
    {
        // Store the old property value.
        updateRtypeDocument(old_indexdoc, new_indexdoc);
        break;
    }
    case REPLACE:
    {
        // nothing need be done.
        break;
    }
    default:
        return false;
    }
    return true;
}

void InvertedIndexManager::removeDocument(docid_t docid, time_t timestamp)
{
    izenelib::ir::indexmanager::Indexer::removeDocument(collectionId_, docid);
}

bool InvertedIndexManager::mergeDocument_(
        const Document& olddoc,
        const Document& newdoc,
        IndexerDocument& indexDocument)
{
    if (olddoc.isEmpty())
    {
        LOG(INFO) << "olddoc is empty while merge for IndexDocument";
        return false;
    }
    sf1r::PropertyConfig tempPropertyConfig;
    docid_t newid = newdoc.getId();
    for (Document::property_const_iterator it = olddoc.propertyBegin(); it != olddoc.propertyEnd(); ++it)
    {
        if (!newdoc.hasProperty(it->first))
        {
            ///Properties that only exist within old doc, while not in new doc
            ///Require to prepare for IndexDocument
            tempPropertyConfig.propertyName_ = it->first;
            IndexBundleSchema::const_iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
            if (iter != bundleConfig_->indexSchema_.end())
            {
                const Document::doc_prop_value_strtype& propValue = olddoc.property(it->first).getPropertyStrValue();
                if (propValue.empty()) continue;
                if (iter->getType() == STRING_PROPERTY_TYPE ||
                    iter->getType() == SUBDOC_PROPERTY_TYPE)
                {
                    prepareIndexDocumentStringProperty_(newid, it->first, propValue, iter, indexDocument);
                }
                else
                    prepareIndexDocumentNumericProperty_(newid, propValue, iter, indexDocument);
            }
        }
    }
    return true;
}

}

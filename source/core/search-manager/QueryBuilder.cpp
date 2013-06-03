#include "QueryBuilder.h"
#include "TermDocumentIterator.h"
#include "SearchTermDocumentIterator.h"
#include "RankTermDocumentIterator.h"
#include "ANDDocumentIterator.h"
#include "ORDocumentIterator.h"
#include "NOTDocumentIterator.h"
#include "WildcardDocumentIterator.h"
#include "WildcardPhraseDocumentIterator.h"
#include "ExactPhraseDocumentIterator.h"
#include "OrderedPhraseDocumentIterator.h"
#include "NearbyPhraseDocumentIterator.h"
#include "PersonalSearchDocumentIterator.h"
#include "VirtualTermDocumentIterator.h"
#include "FilterCache.h"

#include <common/TermTypeDetector.h>

#include <ir/index_manager/utility/BitVector.h>

#include <util/get.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include <boost/token_iterator.hpp>
#include <boost/scoped_ptr.hpp>

//#define VERBOSE_SERACH_MANAGER 1

using namespace std;

using izenelib::ir::indexmanager::TermDocFreqs;
using izenelib::ir::indexmanager::TermReader;

namespace sf1r
{

QueryBuilder::QueryBuilder(
    const boost::shared_ptr<DocumentManager> documentManager,
    const boost::shared_ptr<IndexManager> indexManager,
    const schema_map& schemaMap,
    size_t filterCacheNum
)
    :documentManagerPtr_(documentManager)
    ,indexManagerPtr_(indexManager)
    ,pIndexReader_(indexManager->pIndexReader_)
    ,schemaMap_(schemaMap)
    ,filterCache_(new FilterCache(filterCacheNum))
{
}

QueryBuilder::~QueryBuilder()
{
}

void QueryBuilder::reset_cache()
{
    filterCache_->clear();
}

void QueryBuilder::prepare_filter(
    const std::vector<QueryFiltering::FilteringType>& filtingList,
    boost::shared_ptr<IndexManager::FilterBitmapT>& pFilterBitmap)
{
    if (filtingList.size() == 1)
    {
        const QueryFiltering::FilteringType& filteringItem= filtingList[0];
        QueryFiltering::FilteringOperation filterOperation = filteringItem.operation_;
        const std::string& property = filteringItem.property_;
        const std::vector<PropertyValue>& filterParam = filteringItem.values_;
        if (!filterCache_->get(filteringItem, pFilterBitmap))
        {
            pFilterBitmap.reset(new IndexManager::FilterBitmapT);
            indexManagerPtr_->makeRangeQuery(filterOperation, property, filterParam, pFilterBitmap);
            filterCache_->set(filteringItem, pFilterBitmap);
        }
    }
    else
    {
        const unsigned int bitsNum = pIndexReader_->maxDoc() + 1;
        const unsigned int wordBitNum = sizeof(IndexManager::FilterWordT) << 3;
        const unsigned int wordsNum = (bitsNum - 1) / wordBitNum + 1;

        pFilterBitmap.reset(new IndexManager::FilterBitmapT);
        pFilterBitmap->addStreamOfEmptyWords(true, wordsNum);
        boost::shared_ptr<IndexManager::FilterBitmapT> pFilterBitmap2;
        boost::shared_ptr<IndexManager::FilterBitmapT> pFilterBitmap3;

        try
        {
            std::vector<QueryFiltering::FilteringType>::const_iterator iter = filtingList.begin();
            for (; iter != filtingList.end(); ++iter)
            {
                QueryFiltering::FilteringOperation filterOperation = iter->operation_;
                const std::string& property = iter->property_;
                const std::vector<PropertyValue>& filterParam = iter->values_;
                if (!filterCache_->get(*iter, pFilterBitmap2))
                {
                    pFilterBitmap2.reset(new IndexManager::FilterBitmapT);
                    indexManagerPtr_->makeRangeQuery(filterOperation, property, filterParam, pFilterBitmap2);
                    filterCache_->set(*iter, pFilterBitmap2);
                }
                pFilterBitmap3.reset(new IndexManager::FilterBitmapT);
                if (iter->logic_ == QueryFiltering::OR)
                {
                    (*pFilterBitmap).logicalor(*pFilterBitmap2, *pFilterBitmap3);
                }
                else // if (iter->logic_ == QueryFiltering::AND)
                {
                    (*pFilterBitmap).logicaland(*pFilterBitmap2, *pFilterBitmap3);
                }
                (*pFilterBitmap).swap(*pFilterBitmap3);
            }
        }
        catch (std::exception& e)
        {
        }
    }
}

WANDDocumentIterator* QueryBuilder::prepare_wand_dociterator(
    const SearchKeywordOperation& actionOperation,
    collectionid_t colID,
    const property_weight_map& propertyWeightMap,
    const std::vector<std::string>& properties,
    const std::vector<unsigned int>& propertyIds,
    bool readPositions,
    const std::vector<std::map<termid_t, unsigned> >& termIndexMaps
)
{
    /*size_t size_of_properties = propertyIds.size();

    WANDDocumentIterator* pWandScorer = new WANDDocumentIterator(
        propertyWeightMap,
        propertyIds,
        properties);
    if (pIndexReader_->isDirty())
    {
        pIndexReader_ = indexManagerPtr_->getIndexReader();
    }
    try
    {
        size_t success_properties = 0;
        for (size_t i = 0; i < size_of_properties; i++)
        {
            prepare_for_wand_property_(
                pWandScorer,
                success_properties,
                actionOperation,
                colID,
                properties[i],
                propertyIds[i],
                readPositions,
                termIndexMaps[i]
            );
        }

        if (success_properties)
            return pWandScorer;
        else
            delete pWandScorer;

        return NULL;
    }
    catch(std::exception& e)
    {
        delete pWandScorer;
        throw std::runtime_error("Failed to prepare wanddociterator");
        return NULL;
    }*/
}

void QueryBuilder::prepare_for_wand_property_(
    WANDDocumentIterator* pWandScorer,
    size_t & success_properties,
    const SearchKeywordOperation& actionOperation,
    collectionid_t colID,
    const std::string& property,
    unsigned int propertyId,
    bool readPositions,
    const std::map<termid_t, unsigned>& termIndexMapInProperty
)
{
    typedef std::map<termid_t, unsigned>::const_iterator const_iter;
    schema_iterator found = schemaMap_.find(property);
    if (found == schemaMap_.end())
        return;

    TermReader* pTermReader = NULL;
    pTermReader = pIndexReader_->getTermReader(colID);
    if (!pTermReader)
        return;

    const_iter iter = termIndexMapInProperty.begin();
    for ( ; iter != termIndexMapInProperty.end(); ++iter)
    {
        termid_t termId = iter->first;
        unsigned int termIndex = iter->second;
        Term term(property.c_str(),termId);
        bool find = pTermReader->seek(&term);

        if (find)
        {
            TermDocFreqs* pTermDocReader = 0;
            if (readPositions)
                pTermDocReader = pTermReader->termPositions();
            else
                pTermDocReader = pTermReader->termDocFreqs();
            if(pTermDocReader) ///NULL when exception
            {
                TermDocumentIterator* pIterator = NULL;
                pIterator =
                    new TermDocumentIterator(termId,
                                             colID,
                                             pIndexReader_,
                                             property,
                                             propertyId,
                                             termIndex,
                                             readPositions);
                pIterator->set( pTermDocReader );
                pWandScorer->add(pIterator);
                ++success_properties;
            }
        }
    }
    if (pTermReader)
        delete pTermReader;
}

MultiPropertyScorer* QueryBuilder::prepare_dociterator(
    const SearchKeywordOperation& actionOperation,
    collectionid_t colID,
    const property_weight_map& propertyWeightMap,
    const std::vector<std::string>& properties,
    const std::vector<unsigned int>& propertyIds,
    bool readPositions,
    const std::vector<std::map<termid_t, unsigned> >& termIndexMaps
)
{
    size_t size_of_properties = propertyIds.size();
    std::auto_ptr<MultiPropertyScorer> docIterPtr(new MultiPropertyScorer(propertyWeightMap, propertyIds));
    if (pIndexReader_->isDirty())
    {
        pIndexReader_ = indexManagerPtr_->getIndexReader();
    }

    size_t success_properties = 0;

    for (size_t i = 0; i < size_of_properties; i++)
    {
        schema_iterator found = schemaMap_.find(properties[i]);
        if (found == schemaMap_.end())
            continue;
        if(found->second.subProperties_.empty())
            prepare_for_property_(
                docIterPtr.get(),
                success_properties,
                actionOperation,
                colID,
                found->second,
                readPositions,
                termIndexMaps[i]
            );
        else
            prepare_for_virtual_property_(
                docIterPtr.get(),
                success_properties,
                actionOperation,
                colID,
                found->second,
                readPositions,
                termIndexMaps[i],
                propertyWeightMap
            );
    }

    if (success_properties)
        return docIterPtr.release();

    return NULL;
}

void QueryBuilder::prefetch_term_doc_readers_(
    const std::vector<pair<termid_t, string> >& termList,
    collectionid_t colID,
    const PropertyConfig& properyConfig,
    bool readPositions,
    bool& isNumericFilter,
    std::map<termid_t, std::vector<TermDocFreqs*> > & termDocReaders
)
{
    sf1r::PropertyDataType dataType;

    dataType = properyConfig.getType();
    if( properyConfig.isIndex() &&
            properyConfig.getIsFilter() &&
            properyConfig.getType() != STRING_PROPERTY_TYPE)
        isNumericFilter = true;

    const string& property = properyConfig.getName();

#if PREFETCH_TERMID
    boost::scoped_ptr<TermReader> pTermReader;
    if (!isNumericFilter)
    {
        pTermReader.reset(pIndexReader_->getTermReader(colID));
        if (!pTermReader)
            return;
    }

    for (std::vector<pair<termid_t, string> >::const_iterator it = termList.begin();
            it != termList.end(); ++it)
    {
        termid_t termId = (*it).first;
        const string& termStr = (*it).second;
        if(!isNumericFilter)
        {
            Term term(property.c_str(),termId);
            bool find = pTermReader->seek(&term);

            if (find)
            {
                TermDocFreqs* pTermDocReader = 0;
                if (readPositions)
                    pTermDocReader = pTermReader->termPositions();
                else
                    pTermDocReader = pTermReader->termDocFreqs();
                if(pTermDocReader) ///NULL when exception
                    termDocReaders[termId].push_back(pTermDocReader);
            }
        }
        else
        {
            boost::shared_ptr<IndexManager::FilterBitmapT> pFilterBitmap;
            boost::shared_ptr<BitVector> pBitVector;

            if (!TermTypeDetector::isTypeMatch(termStr, dataType))
                continue;

            PropertyType value;
            PropertyValue2IndexPropertyType converter(value);
            boost::apply_visitor(converter, TermTypeDetector::propertyValue_.getVariant());
            bool find = indexManagerPtr_->seekTermFromBTreeIndex(colID, property, value);

            if (find)
            {
                QueryFiltering::FilteringType filteringRule;
                filteringRule.operation_ = QueryFiltering::EQUAL;
                filteringRule.property_ = property;
                std::vector<PropertyValue> filterParam;
                filterParam.push_back(TermTypeDetector::propertyValue_);
                filteringRule.values_ = filterParam;
                if(!filterCache_->get(filteringRule, pFilterBitmap))
                {
                    pFilterBitmap.reset(new IndexManager::FilterBitmapT);
                    pBitVector.reset(new BitVector(pIndexReader_->maxDoc() + 1));

                    indexManagerPtr_->getDocsByNumericValue(colID, property, value, *pBitVector);
                    pBitVector->compressed(*pFilterBitmap);
                    filterCache_->set(filteringRule, pFilterBitmap);
                }
                TermDocFreqs* pTermDocReader = new IndexManager::FilterTermDocFreqsT(pFilterBitmap);
                termDocReaders[termId].push_back(pTermDocReader);
            }
        }
    }
#endif
}

void QueryBuilder::prepare_for_property_(
    MultiPropertyScorer* pScorer,
    size_t & success_properties,
    const SearchKeywordOperation& actionOperation,
    collectionid_t colID,
    const PropertyConfig& properyConfig,
    bool readPositions,
    const std::map<termid_t, unsigned>& termIndexMapInProperty
)
{
    std::map<termid_t, std::vector<TermDocFreqs*> > termDocReaders;
    bool isNumericFilter = false;
    std::vector<pair<termid_t, string> > termList;
    actionOperation.getQueryTermInfoList(properyConfig.getName(), termList);
    std::sort(termList.begin(), termList.end());
    prefetch_term_doc_readers_(termList,colID,properyConfig,readPositions,isNumericFilter,termDocReaders);
    sf1r::PropertyDataType dataType = properyConfig.getType();
    const string& property = properyConfig.getName();
    unsigned propertyId = properyConfig.getPropertyId();
    std::map<termid_t, VirtualPropertyTermDocumentIterator* > virtualTermIters;
    DocumentIterator* pIter = NULL;
    QueryTreePtr queryTree;
    if ( actionOperation.getQueryTree(property, queryTree) )
        do_prepare_for_property_(
            queryTree,
            colID,
            property,
            propertyId,
            dataType,
            isNumericFilter,
            readPositions,
            termIndexMapInProperty,
            pIter,
            virtualTermIters,
            termDocReaders,
            actionOperation.hasUnigramProperty_,
            actionOperation.isUnigramSearchMode_
        );

    if (pIter)
    {
        pScorer->add(propertyId, pIter, termDocReaders);
        ++success_properties;
    }
    else
    {
        for (std::map<termid_t, std::vector<TermDocFreqs*> >::iterator
                it = termDocReaders.begin(); it != termDocReaders.end(); ++it)
        {
            for (size_t j =0; j<it->second.size(); j ++ )
            {
                delete it->second[j];
            }
            it->second.clear();
        }
    }
}

void QueryBuilder::post_prepare_ranker_(
    const std::string& virtualProperty,
    const std::vector<std::string>& indexPropertyList,
    unsigned indexPropertySize,
    const property_term_info_map& propertyTermInfoMap,
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties& ctfmap,
    MaxTermFrequencyInProperties& maxtfmap,
    bool readTermPosition,
    std::vector<RankQueryProperty>& rankQueryProperties,
    std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers)
{
    static const PropertyTermInfo emptyPropertyTermInfo;
    for (unsigned i = 0; i < indexPropertySize; ++i)
    {
        const std::string& currentProperty = indexPropertyList[i];
        schema_iterator found = schemaMap_.find(currentProperty);
        if (found == schemaMap_.end())
            continue;
        if (found->second.subProperties_.empty())
        {

            rankQueryProperties[i].setNumDocs(
                indexManagerPtr_->numDocs()
            );

            rankQueryProperties[i].setTotalPropertyLength(
                documentManagerPtr_->getTotalPropertyLength(currentProperty)
            );
            const PropertyTermInfo::id_uint_list_map_t& termPositionsMap = virtualProperty.empty()?
                    izenelib::util::getOr(
                        propertyTermInfoMap,
                        currentProperty,
                        emptyPropertyTermInfo
                    ).getTermIdPositionMap()
                    :
                    izenelib::util::getOr(
                        propertyTermInfoMap,
                        virtualProperty,
                        emptyPropertyTermInfo
                    ).getTermIdPositionMap()
                    ;

            typedef PropertyTermInfo::id_uint_list_map_t::const_iterator
            term_id_position_iterator;
            unsigned queryLength = 0;
            unsigned index = 0;
            for (term_id_position_iterator termIt = termPositionsMap.begin();
                    termIt != termPositionsMap.end(); ++termIt, ++index)
            {
                rankQueryProperties[i].addTerm(termIt->first);
                rankQueryProperties[i].setTotalTermFreq(
                    ctfmap[currentProperty][termIt->first]
                );
                rankQueryProperties[i].setDocumentFreq(
                    dfmap[currentProperty][termIt->first]
                );

                rankQueryProperties[i].setMaxTermFreq(
                    maxtfmap[currentProperty][termIt->first]
                );

                queryLength += termIt->second.size();
                if (readTermPosition)
                {
                    typedef PropertyTermInfo::id_uint_list_map_t::mapped_type
                    uint_list_map_t;
                    typedef uint_list_map_t::const_iterator uint_list_iterator;
                    for (uint_list_iterator posIt = termIt->second.begin();
                            posIt != termIt->second.end(); ++posIt)
                    {
                        rankQueryProperties[i].pushPosition(*posIt);
                    }
                }
                else
                {
                    rankQueryProperties[i].setTermFreq(termIt->second.size());
                }
            }

            rankQueryProperties[i].setQueryLength(queryLength);
        }
        else
        {
            rankQueryProperties[i].setNumDocs(
                indexManagerPtr_->numDocs());
///cout<<"indexManagerPtr_->numDocs()"<<indexManagerPtr_->numDocs()<<endl;
            const PropertyTermInfo::id_uint_list_map_t& termPositionsMap = izenelib::util::getOr(
                        propertyTermInfoMap,
                        currentProperty,
                        emptyPropertyTermInfo
                    ).getTermIdPositionMap();
            uint32_t DocumentFreq = 0;
            uint32_t MaxTermFreq = 0; 
            uint32_t index = 0;
            uint32_t queryLength = 0;
            uint32_t ctf = 0;
            uint32_t TotalPropertyLength = 0;
            typedef PropertyTermInfo::id_uint_list_map_t::const_iterator
            term_id_position_iterator;
            for (term_id_position_iterator termIt = termPositionsMap.begin();
                termIt != termPositionsMap.end(); ++termIt)
            {
                rankQueryProperties[i].addTerm(termIt->first);
                queryLength += termIt->second.size();
                for (uint32_t j = 0; j < found->second.subProperties_.size(); ++j, ++index)
                {
                    uint32_t dftmp = dfmap[found->second.subProperties_[index]][termIt->first];// subString and terid;
                    uint32_t ctftmp = ctfmap[found->second.subProperties_[index]][termIt->first];
                    uint32_t maxtf =  maxtfmap[found->second.subProperties_[index]][termIt->first];
                    DocumentFreq += dftmp;
                    ctf += ctftmp;
                    if ( maxtf > MaxTermFreq)
                    {
                        MaxTermFreq = maxtf;
                    }
                    if (readTermPosition)
                    {
                        /**
                        code
                        */
                    }
                    if (termIt == termPositionsMap.begin())
                    {
                        TotalPropertyLength += documentManagerPtr_->getTotalPropertyLength(found->second.subProperties_[j]);
                    }
                }
                rankQueryProperties[i].setTermFreq(termIt->second.size());
///cout<<"MaxTermFreq"<<MaxTermFreq<<endl;
///cout<<"DocumentFreq"<<DocumentFreq<<endl;
///cout<<"ctf"<<ctf<<endl;//该属性所有包含该TermID的频率值;
                rankQueryProperties[i].setMaxTermFreq(MaxTermFreq);
                MaxTermFreq = 0;
                rankQueryProperties[i].setDocumentFreq(DocumentFreq);
                DocumentFreq = 0;
                rankQueryProperties[i].setTotalTermFreq(ctf);
                ctf = 0;
                index = 0;
            }

            rankQueryProperties[i].setTotalPropertyLength(TotalPropertyLength);
            TotalPropertyLength = 0;
///cout<<"virtual queryLength"<<queryLength<<endl;
            rankQueryProperties[i].setQueryLength(queryLength);
            queryLength = 0;
        }
    }

    for (size_t i = 0; i < indexPropertySize; ++i )
    {
        propertyRankers[i]->setupStats(rankQueryProperties[i]);
    }
}

void QueryBuilder::prepare_for_virtual_property_(
    MultiPropertyScorer* pScorer,
    size_t & success_properties,
    const SearchKeywordOperation& actionOperation,
    collectionid_t colID,
    const PropertyConfig& properyConfig,
    bool readPositions,
    const std::map<termid_t, unsigned>& termIndexMap,
    const property_weight_map& propertyWeightMap
    )
{
    LOG(INFO)<<"start prepare_for_virtual_property_"<<endl;
    QueryTreePtr queryTree;
    if (! actionOperation.getQueryTree(properyConfig.getName(), queryTree) ) 
        return;
    DocumentIterator* pIter = NULL;

    std::vector<pair<termid_t, string> > termList;
    actionOperation.getQueryTermInfoList(properyConfig.getName(), termList);
    std::sort(termList.begin(), termList.end());
    std::vector<std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> > > termDocReadersList;
    termDocReadersList.resize(properyConfig.subProperties_.size());

    std::vector<std::string> properties;
    std::vector<unsigned int> propertyIds;
    std::vector<PropertyDataType> propertyDataTypes;
    bool isNumericFilter = false;

    for(unsigned j = 0; j < properyConfig.subProperties_.size(); ++j)
    {
        schema_iterator p = schemaMap_.find(properyConfig.subProperties_[j]);
        prefetch_term_doc_readers_(termList, colID, p->second, readPositions, isNumericFilter, termDocReadersList[j]);
        properties.push_back(p->second.getName());
        propertyIds.push_back(p->second.getPropertyId());
        propertyDataTypes.push_back(p->second.getType());
    }
    
    bool ret = do_prepare_for_virtual_property_(
                queryTree,
                colID,
                properties,
                propertyIds,
                propertyDataTypes,
                isNumericFilter,
                readPositions,
                termIndexMap,
                pIter,
                termDocReadersList,
                actionOperation.hasUnigramProperty_,
                actionOperation.isUnigramSearchMode_
            );

    if (!ret)
    {
        for(unsigned j = 0; j < properyConfig.subProperties_.size(); ++j)
        {
            for (std::map<termid_t, std::vector<TermDocFreqs*> >::iterator
                    it = termDocReadersList[j].begin(); it != termDocReadersList[j].end(); ++it)
            {
                for (size_t i =0; i < it->second.size(); ++i )
                {
                    delete it->second[i];
                }
                it->second.clear();
            }
        }
        return;
    }

    if (pIter)
    {
        pScorer->add(properyConfig.getPropertyId(), pIter);
        pScorer->add(termDocReadersList);
        success_properties++;
    }
    else
    {
        for(unsigned j = 0; j < properyConfig.subProperties_.size(); ++j)
        {
            for (std::map<termid_t, std::vector<TermDocFreqs*> >::iterator
                    it = termDocReadersList[j].begin(); it != termDocReadersList[j].end(); ++it)
            {
                for (size_t i =0; i < it->second.size(); ++i )
                {
                    delete it->second[i];
                }
                it->second.clear();
            }
        }
    }
}

bool QueryBuilder::do_prepare_for_virtual_property_(
    QueryTreePtr& queryTree, 
    collectionid_t colID,
    const std::vector<std::string>& properties,
    std::vector<unsigned int>& propertyIds,
    std::vector<PropertyDataType>& propertyDataTypes,
    bool isNumericFilter,
    bool isReadPosition,
    const std::map<termid_t, unsigned>& termIndexMap,
    DocumentIteratorPointer& pDocIterator,
    std::vector<std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> > >& termDocReadersList,
    bool hasUnigramProperty,
    bool isUnigramSearchMode,
    int parentAndOrFlag
)
{
    switch (queryTree->type_)
    {
    case QueryTree::KEYWORD:
    {
        string keyword = queryTree->keyword_;
        termid_t keywordId = queryTree->keywordId_;

        unsigned termIndex = izenelib::util::getOr(
                                     termIndexMap,
                                     keywordId,
                                     (std::numeric_limits<unsigned>::max) ()
                                 );

        VirtualTermDocumentIterator* pVirtualTermDocIter = new VirtualTermDocumentIterator(
            keywordId,
            keyword,
            colID,
            pIndexReader_,
            indexManagerPtr_,
            properties,
            propertyIds,
            propertyDataTypes,
            termIndex,
            isReadPosition
        );
        DocumentIterator* pIterator = new ORDocumentIterator();
        try
        {
            for( uint32_t i = 0; i < properties.size(); ++i)
            {
                TermDocumentIterator* pTermIterator = NULL;
                
                if (!isUnigramSearchMode)
                {
                    pTermIterator = new TermDocumentIterator(
                        keywordId,
                        keyword,
                        colID,
                        pIndexReader_,
                        indexManagerPtr_,
                        properties[i],
                        propertyIds[i],
                        propertyDataTypes[i],
                        isNumericFilter,
                        termIndex,
                        isReadPosition);
                }
                else
                {
                    if (queryTree->type_ == QueryTree::KEYWORD)
                    {
                        pTermIterator = new SearchTermDocumentIterator(keywordId,
                                colID,
                                pIndexReader_,
                                properties[i],
                                propertyIds[i],
                                termIndex,
                                isReadPosition);
                    }
                }

                std::map<termid_t, std::vector<TermDocFreqs*> >::iterator constIt
                = termDocReadersList[i].find(keywordId);

                if (constIt != termDocReadersList[i].end() && !constIt->second.empty() )
                {
                    pVirtualTermDocIter->set(constIt->second.back() );
                    pTermIterator->set(constIt->second.back() );
                    if(pIterator == NULL)
                        pIterator = pTermIterator;
                    else
                        pIterator->add(pTermIterator);

                    constIt->second.pop_back();
                }
                else
                {
                    delete pTermIterator;
                    pTermIterator = NULL;
                    pVirtualTermDocIter->set(0);
                }
            }

            pVirtualTermDocIter->add(pIterator);
            if (pDocIterator == NULL)
            {
                pDocIterator = pVirtualTermDocIter;
            }
            else
                pDocIterator->add(pVirtualTermDocIter);
        }
        catch(std::exception& e)
        {
            LOG (ERROR) << "exception in virtual keyword search...";
            delete pIterator;
            return false;
        }
        return true;
        break;
    }

    case QueryTree::AND:
    {
        DocumentIterator* pIterator = new ANDDocumentIterator();
        try
        {
            for (QTIter andChildIter = queryTree->children_.begin();
                    andChildIter != queryTree->children_.end(); ++andChildIter)
            {
                bool ret = do_prepare_for_virtual_property_(
                            *andChildIter,
                            colID,
                            properties,
                            propertyIds,
                            propertyDataTypes,
                            isNumericFilter,
                            isReadPosition,
                            termIndexMap,
                            pIterator,
                            termDocReadersList,
                            hasUnigramProperty,
                            isUnigramSearchMode,
                            1
                            );
                if (!ret)
                {
                    delete pIterator;
                    return false;
                }
            }
            if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);

        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    }

    case QueryTree::NOT:
    {
        if ( queryTree->children_.size() != 1 )
            return false;
        if (NULL == pDocIterator)
            ///only NOT is not permitted
            return false;
        try
        {
            QTIter notChildIter = queryTree->children_.begin();// more is a xunhuan...
            if (notChildIter == queryTree->children_.end())
                return false;
            DocumentIterator* pIterator = NULL;
            if ((*notChildIter)->type_ == QueryTree::NOT)
            {
                return false;
            }// do not support: NOT NOT like !(!())
            do_prepare_for_virtual_property_(
                            *notChildIter,
                            colID,
                            properties,
                            propertyIds,
                            propertyDataTypes,
                            isNumericFilter,
                            isReadPosition,
                            termIndexMap,
                            pIterator,
                            termDocReadersList,
                            hasUnigramProperty,
                            isUnigramSearchMode,
                            1
                        );
            if (pIterator)
            {
                pIterator->setNot(true);
                pDocIterator->add(pIterator);
            }
        }
        catch(std::exception& e)
        {
            //delete pIterator;
            return true;
        }
        break;
    }

    case QueryTree::OR:
    {
        DocumentIterator* pIterator = new ORDocumentIterator();
        try
        {
            for (QTIter andChildIter = queryTree->children_.begin();
                    andChildIter != queryTree->children_.end(); ++andChildIter)
            {
                bool ret = do_prepare_for_virtual_property_(
                            *andChildIter,
                            colID,
                            properties,
                            propertyIds,
                            propertyDataTypes,
                            isNumericFilter,
                            isReadPosition,
                            termIndexMap,
                            pIterator,
                            termDocReadersList,
                            hasUnigramProperty,
                            isUnigramSearchMode,
                            1
                            );
                if (!ret)
                {
                    delete pIterator;
                    return false;
                }
            }
            //parentAndorFlag 
            if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    }

    default:
        break;
    }
    return true;
}

bool QueryBuilder::do_prepare_for_property_(
    QueryTreePtr& queryTree,
    collectionid_t colID,
    const std::string& property,
    unsigned int propertyId,
    PropertyDataType propertyDataType,
    bool isNumericFilter,
    bool readPositions,
    const std::map<termid_t, unsigned>& termIndexMapInProperty,
    DocumentIteratorPointer& pDocIterator,
    std::map<termid_t, VirtualPropertyTermDocumentIterator* >& virtualTermIters,
    std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >& termDocReaders,
    bool hasUnigramProperty,
    bool isUnigramSearchMode,
    const std::string& virtualProperty,
    int parentAndOrFlag
)
{
    switch (queryTree->type_)
    {
    case QueryTree::KEYWORD:
    case QueryTree::RANK_KEYWORD:
    {
        string keyword = queryTree->keyword_;
        termid_t keywordId = queryTree->keywordId_;
        unsigned termIndex = izenelib::util::getOr(
                                 termIndexMapInProperty,
                                 keywordId,
                                 (std::numeric_limits<unsigned>::max) ()
                             );

        TermDocumentIterator* pIterator = NULL;
        try
        {
            if (!isUnigramSearchMode)
            {
                pIterator = new TermDocumentIterator(
                    keywordId,
                    keyword,
                    colID,
                    pIndexReader_,
                    indexManagerPtr_,
                    property,
                    propertyId,
                    propertyDataType,
                    isNumericFilter,
                    termIndex,
                    readPositions);
            }
            else
            {
                // term for searching
                if (queryTree->type_ == QueryTree::KEYWORD)
                {
                    // termIndex is invalid in this case, it will not be used
                    pIterator = new SearchTermDocumentIterator(keywordId,
                            colID,
                            pIndexReader_,
                            property,
                            propertyId,
                            termIndex,
                            readPositions);
                }
                // term for ranking
                else //if(queryTree->type_ == QueryTree::RANK_KEYWORD)
                {
                    pIterator = new RankTermDocumentIterator(keywordId,
                            colID,
                            pIndexReader_,
                            property,
                            propertyId,
                            termIndex,
                            readPositions,
                            (parentAndOrFlag == 1));
                }
            }

#if PREFETCH_TERMID
            std::map<termid_t, std::vector<TermDocFreqs*> >::iterator constIt
            = termDocReaders.find(keywordId);
            if (constIt != termDocReaders.end() && !constIt->second.empty() )
            {
#ifdef VERBOSE_SERACH_MANAGER
                cout<<"have term  property "<<property<<" keyword ";
                queryTree->keywordUString_.displayStringValue(izenelib::util::UString::UTF_8);
                cout<<endl;
#endif
                pIterator->set(constIt->second.back() );
                if(virtualProperty.empty())
                {
                    ///General keyword search
                    if (NULL == pDocIterator)
                        pDocIterator = pIterator;
                    else
                        pDocIterator->add(pIterator);
                }
                else
                {
                    ///Keyword search over virtual property
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vpIt =
                        virtualTermIters.find(keywordId);
                    if(vpIt != virtualTermIters.end())
                    {
                        vpIt->second->add(pIterator);
                    }
                    else
                    {
                        VirtualPropertyTermDocumentIterator* pVirtualTermDocIter =
                            new VirtualPropertyTermDocumentIterator(virtualProperty);
                        pVirtualTermDocIter->add(pIterator);
                        virtualTermIters[keywordId] = pVirtualTermDocIter;
                        if (NULL == pDocIterator)
                            pDocIterator = pVirtualTermDocIter;
                    }
                }

                constIt->second.pop_back();
            }
            else
#endif
            {
                if (pIterator->accept())
                {
#ifdef VERBOSE_SERACH_MANAGER
                    cout<<"have term  property "<<property<<" keyword ";
                    queryTree->keywordUString_.displayStringValue(izenelib::util::UString::UTF_8);
                    cout<<endl;
#endif
                    if(virtualProperty.empty())
                    {
                        ///General keyword search
                        if (NULL == pDocIterator)
                            pDocIterator = pIterator;
                        else
                            pDocIterator->add(pIterator);
                    }
                    else
                    {
                        ///Keyword search over virtual property
                        std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vpIt =
                            virtualTermIters.find(keywordId);
                        if(vpIt != virtualTermIters.end())
                        {
                            vpIt->second->add(pIterator);
                        }
                        else
                        {
                            VirtualPropertyTermDocumentIterator* pVirtualTermDocIter =
                                new VirtualPropertyTermDocumentIterator(virtualProperty);
                            pVirtualTermDocIter->add(pIterator);
                            virtualTermIters[keywordId] = pVirtualTermDocIter;
                            if (NULL == pDocIterator)
                                pDocIterator = pVirtualTermDocIter;
                        }
                    }

                }
                else
                {
#ifdef VERBOSE_SERACH_MANAGER
                    cout<<"do not have term property "<<property<<" ";
                    queryTree->keywordUString_.displayStringValue(izenelib::util::UString::UTF_8);
                    cout<<endl;
#endif
                    delete pIterator;
                    pIterator = NULL;
                    if (!isNumericFilter)
                        return false;
                }
            }
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        return true;
        break;
    } // end - QueryTree::KEYWORD
    case QueryTree::NOT:
    {
        if ( queryTree->children_.size() != 1 )
            return false;
        if (NULL == pDocIterator)
            ///only NOT is not permitted
            return false;
        try
        {
            QTIter notChildIter = queryTree->children_.begin();// more is a xunhuan...
            if (notChildIter == queryTree->children_.end())
                return false;
            DocumentIterator* pIterator = NULL;
            if ((*notChildIter)->type_ == QueryTree::NOT)
            {
                return false;
            }
            do_prepare_for_property_(
                          *notChildIter,
                          colID,
                          property,
                          propertyId,
                          propertyDataType,
                          isNumericFilter,
                          readPositions,
                          termIndexMapInProperty,
                          pIterator,
                          virtualTermIters,
                          termDocReaders,
                          hasUnigramProperty,
                          isUnigramSearchMode,
                          virtualProperty,
                          1
                      );
            if (pIterator)
            {
                pIterator->setNot(true);
                pDocIterator->add(pIterator);
            }
        }
        catch(std::exception& e)
        {
            //delete pIterator;
            return true;
        }
        break;
    } // end QueryTree::NOT
    case QueryTree::UNIGRAM_WILDCARD:
    {
        vector<termid_t> termIds;
        vector<unsigned> termIndexes;
        vector<size_t> asterisk_pos;
        vector<size_t> question_mark_pos;
        for (std::list<QueryTreePtr>::iterator childIter = queryTree->children_.begin();
                childIter != queryTree->children_.end(); ++childIter)
        {
            if ((*childIter)->type_ == QueryTree::ASTERISK )
            {
                asterisk_pos.push_back( termIds.size() );
            }
            else if ((*childIter)->type_ == QueryTree::QUESTION_MARK )
            {
                question_mark_pos.push_back( termIds.size() );
            }
            else
            {
                termid_t keywordId( (*childIter)->keywordId_ );
                unsigned termIndex = izenelib::util::getOr(
                                         termIndexMapInProperty,
                                         keywordId,
                                         (std::numeric_limits<unsigned>::max) ()
                                     );
                termIds.push_back(keywordId);
                termIndexes.push_back(termIndex);
            }
        }

        // if there is no unigram alias-ed property, perform on original property
        std::string unigramProperty = property;
        if (hasUnigramProperty)
        {
            unigramProperty += "_unigram";
        }
        unsigned int unigramPropertyId = 0;
        schema_iterator found = schemaMap_.find(unigramProperty);
        if (found != schemaMap_.end())
        {
            unigramPropertyId = found->second.getPropertyId();
        }
        WildcardPhraseDocumentIterator* pIterator =
            new WildcardPhraseDocumentIterator(
            termIds,
            termIndexes,
            asterisk_pos,
            question_mark_pos,
            colID,
            pIndexReader_,
            unigramProperty,
            unigramPropertyId,
            documentManagerPtr_
        );
        if(NULL == pDocIterator)
            pDocIterator = pIterator;
        else
            pDocIterator->add(pIterator);

        break;
    } // end - QueryTree::UNIGRAM_WILDCARD
    case QueryTree::TRIE_WILDCARD:
    {
        if (queryTree->children_.size() == 0)
            return false;

        //5 top frequent terms
        static const int kWildcardTermThreshold = 5;
        WildcardDocumentIterator* pWildCardDocIter =
            new WildcardDocumentIterator(
            colID,
            pIndexReader_,
            property,
            propertyId,
            readPositions,
            kWildcardTermThreshold
        );
        for (QTIter iter = queryTree->children_.begin();
                iter != queryTree->children_.end(); ++iter)
        {
            termid_t termId = (*iter)->keywordId_;
            unsigned termIndex = izenelib::util::getOr(
                                     termIndexMapInProperty,
                                     termId,
                                     (std::numeric_limits<unsigned>::max) ()
                                 );

            pWildCardDocIter->add(termId, termIndex, termDocReaders);
        }
        if (0 == pWildCardDocIter->numIterators())
        {
            delete pWildCardDocIter;
            return false;
        }
        if(NULL == pDocIterator)
            pDocIterator = pWildCardDocIter;
        else
            pDocIterator->add(pWildCardDocIter);
        break;
    } // end - QueryTree::TRIE_WILDCARD
    case QueryTree::AND:
    {
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"AND query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new ANDDocumentIterator();
        pIterator->setMissRate(queryTree->children_.size());
        bool ret = false;
        try
        {
            for (QTIter andChildIter = queryTree->children_.begin();
                    andChildIter != queryTree->children_.end(); ++andChildIter)
            {
                ret = do_prepare_for_property_(
                          *andChildIter,
                          colID,
                          property,
                          propertyId,
                          propertyDataType,
                          isNumericFilter,
                          readPositions,
                          termIndexMapInProperty,
                          pIterator,
                          virtualTermIters,
                          termDocReaders,
                          hasUnigramProperty,
                          isUnigramSearchMode,
                          virtualProperty,
                          1
                      );
                if (virtualProperty.empty()&&!ret)
                {
                    delete pIterator;
                    return false;
                }
            }
            if(!virtualProperty.empty())
            {
                if(!parentAndOrFlag)
                {
                    if(NULL == pDocIterator)
                    {
                        pDocIterator = pIterator;
                    }
                    else delete pIterator;
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vit = virtualTermIters.begin();
                    for(; vit != virtualTermIters.end(); ++vit)
                        pDocIterator->add(vit->second);
                }
            }
            else
                ///General AND query
                if (NULL == pDocIterator)
                    pDocIterator = pIterator;
                else
                    pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::AND
    case QueryTree::WAND:
    {
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"WAND query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new WANDDocumentIterator();
        pIterator->setMissRate(queryTree->children_.size());
        bool ret = false;
        try
        {
            for (QTIter wandChildIter = queryTree->children_.begin();
                    wandChildIter != queryTree->children_.end(); ++wandChildIter)
            {
                ret |= do_prepare_for_property_(
                           *wandChildIter,
                           colID,
                           property,
                           propertyId,
                           propertyDataType,
                           isNumericFilter,
                           readPositions,
                           termIndexMapInProperty,
                           pIterator,
                           virtualTermIters,
                           termDocReaders,
                           hasUnigramProperty,
                           isUnigramSearchMode,
                           virtualProperty,
                           1
                       );
            }
            if (!ret)
            {
                delete pIterator;
                return false;
            }
            
            if(!virtualProperty.empty())
            {
                if(!parentAndOrFlag)
                {
                    if(NULL == pDocIterator)
                    {
                        pDocIterator = pIterator;
                    }
                    else delete pIterator;
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vit = virtualTermIters.begin();
                    for(; vit != virtualTermIters.end(); ++vit)
                        pDocIterator->add(vit->second);
                }
            }
            else if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::WAND
    case QueryTree::OR:
    {
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"OR query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new ORDocumentIterator();
        pIterator->setMissRate(queryTree->children_.size());
        bool ret = false;
        try
        {
            for (QTIter orChildIter = queryTree->children_.begin();
                    orChildIter != queryTree->children_.end(); ++orChildIter)
            {
                ret |= do_prepare_for_property_(
                           *orChildIter,
                           colID,
                           property,
                           propertyId,
                           propertyDataType,
                           isNumericFilter,
                           readPositions,
                           termIndexMapInProperty,
                           pIterator,
                           virtualTermIters,
                           termDocReaders,
                           hasUnigramProperty,
                           isUnigramSearchMode,
                           virtualProperty,
                           1
                       );
            }
            if (!ret)
            {
                delete pIterator;
                return false;
            }
            
            if(!virtualProperty.empty())
            {
                if(!parentAndOrFlag)
                {
                    if(NULL == pDocIterator)
                    {
                        pDocIterator = pIterator;
                    }
                    else delete pIterator;
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vit = virtualTermIters.begin();
                    for(; vit != virtualTermIters.end(); ++vit)
                        pDocIterator->add(vit->second);
                }
            }
            else if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::OR
    case QueryTree::AND_PERSONAL:
    {
        /// always return true in this case,
        /// it's tolerable that personal items are not indexed.
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"AND-Personal query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new PersonalSearchDocumentIterator(true);
        bool ret = false;
        try
        {
            for (QTIter andChildIter = queryTree->children_.begin();
                    andChildIter != queryTree->children_.end(); ++andChildIter)
            {
                ret = do_prepare_for_property_(
                          *andChildIter,
                          colID,
                          property,
                          propertyId,
                          propertyDataType,
                          isNumericFilter,
                          readPositions,
                          termIndexMapInProperty,
                          pIterator,
                          virtualTermIters,
                          termDocReaders,
                          hasUnigramProperty,
                          isUnigramSearchMode,
                          virtualProperty
                      );
                if (virtualProperty.empty()&&!ret)
                {
                    delete pIterator;
                    ///return false;
                    return true;
                }
            }
            if(!virtualProperty.empty())
            {
                if(!parentAndOrFlag)
                {
                    if(NULL == pDocIterator)
                    {
                        pDocIterator = pIterator;
                    }
                    else delete pIterator;
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vit = virtualTermIters.begin();
                    for(; vit != virtualTermIters.end(); ++vit)
                        pDocIterator->add(vit->second);
                }
            }
            else if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::AND_PERSONAL
    case QueryTree::OR_PERSONAL:
    {
        /// always return true in this case,
        /// it's tolerable that personal items are not indexed.
        /// OR_PERSONAL iterator is an AND semantic, but a child of OR.
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"OR-Personal query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new PersonalSearchDocumentIterator(false);
        bool ret = false;
        try
        {
            for (QTIter orChildIter = queryTree->children_.begin();
                    orChildIter != queryTree->children_.end(); ++orChildIter)
            {
                ret = do_prepare_for_property_(
                          *orChildIter,
                          colID,
                          property,
                          propertyId,
                          propertyDataType,
                          isNumericFilter,
                          readPositions,
                          termIndexMapInProperty,
                          pIterator,
                          virtualTermIters,
                          termDocReaders,
                          hasUnigramProperty,
                          isUnigramSearchMode,
                          virtualProperty
                      );
                if (!ret)
                {
                    delete pIterator;
                    ///return false;
                    return true;
                }
            }
            if(!virtualProperty.empty())
            {
                if(!parentAndOrFlag)
                {
                    if(NULL == pDocIterator)
                    {
                        pDocIterator = pIterator;
                    }
                    else delete pIterator;
                    std::map<termid_t, VirtualPropertyTermDocumentIterator* >::iterator vit = virtualTermIters.begin();
                    for(; vit != virtualTermIters.end(); ++vit)
                        pDocIterator->add(vit->second);
                }
            }
            else if (NULL == pDocIterator)
                pDocIterator = pIterator;
            else
                pDocIterator->add(pIterator);
        }
        catch(std::exception& e)
        {
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::OR_PERSONAL
    case QueryTree::EXACT:
    {
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"EXACT query "<<property<<endl;
#endif

        vector<termid_t> termIds;
        vector<unsigned> termIndexes;
        getTermIdsAndIndexesOfSiblings(
            queryTree,
            termIndexMapInProperty,
            property,
            termIds,
            termIndexes
        );

        // if there is no unigram alias-ed property, perform on original property
        std::string unigramProperty = property;
        if (hasUnigramProperty)
        {
            unigramProperty += "_unigram";
        }
        unsigned int unigramPropertyId = 0;
        schema_iterator found = schemaMap_.find(unigramProperty);
        if (found != schemaMap_.end())
        {
            unigramPropertyId = found->second.getPropertyId();
        }
        ExactPhraseDocumentIterator* pIterator =
            new ExactPhraseDocumentIterator(
            termIds,
            termIndexes,
            colID,
            pIndexReader_,
            unigramProperty,
            unigramPropertyId
        );
        pIterator->setOrigProperty(property);
        if (NULL == pDocIterator)
            pDocIterator = pIterator;
        else
            pDocIterator->add(pIterator);

        break;
    }  // end - QueryTree::EXACT
    case QueryTree::NEARBY:
    {
        vector<termid_t> termIds;
        vector<unsigned> termIndexes;
        getTermIdsAndIndexesOfSiblings(
            queryTree,
            termIndexMapInProperty,
            property,
            termIds,
            termIndexes
        );

        NearbyPhraseDocumentIterator* pIterator =
            new NearbyPhraseDocumentIterator(
            termIds,
            termIndexes,
            colID,
            pIndexReader_,
            property,
            propertyId,
            queryTree->distance_
        );

        if (NULL == pDocIterator)
            pDocIterator = pIterator;
        else
            pDocIterator->add(pIterator);
        break;
    } // end - QueryTree::NEARBY
    case QueryTree::ORDER:
    {
        vector<termid_t> termIds;
        vector<unsigned> termIndexes;
        getTermIdsAndIndexesOfSiblings(
            queryTree,
            termIndexMapInProperty,
            property,
            termIds,
            termIndexes
        );

        OrderedPhraseDocumentIterator* pIterator =
            new OrderedPhraseDocumentIterator(
            termIds,
            termIndexes,
            colID,
            pIndexReader_,
            property,
            propertyId
        );
        if (NULL == pDocIterator)
            pDocIterator = pIterator;
        else
            pDocIterator->add(pIterator);
        break;
    } // end - QueryTree::ORDER
    default:
        break;
    } // end - switch

    return true;
}

void QueryBuilder::getTermIdsAndIndexesOfSiblings(
    QueryTreePtr& queryTree,
    const std::map<termid_t, unsigned>& termIndexMapInProperty,
    const std::string& property,
    std::vector<termid_t>& outTermIds,
    std::vector<unsigned>& outTermIndexes
)
{
    std::vector<termid_t> termIds;
    std::vector<unsigned> termIndexes;

    for (QTIter childIter = queryTree->children_.begin();
            childIter != queryTree->children_.end(); ++childIter)
    {
        termid_t keywordId( (*childIter)->keywordId_ );
        unsigned termIndex = izenelib::util::getOr(
                                 termIndexMapInProperty,
                                 keywordId,
                                 (std::numeric_limits<unsigned>::max) ()
                             );

        termIds.push_back(keywordId);
        termIndexes.push_back(termIndex);
    }

    termIds.swap(outTermIds);
    termIndexes.swap(outTermIndexes);
}

propertyid_t QueryBuilder::getPropertyIdByName(const std::string& name) const
{
    schema_iterator found = schemaMap_.find(name);
    if (found != schemaMap_.end())
    {
        return found->second.getPropertyId();
    }
    else
    {
        return 0;
    }
}

} // namespace sf1r

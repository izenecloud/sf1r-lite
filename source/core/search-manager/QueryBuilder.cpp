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

#include "FilterCache.h"

#include <common/TermTypeDetector.h>

#include <ir/index_manager/utility/BitVector.h>
#include <ir/index_manager/utility/BitMapIterator.h>

#include <util/get.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include <boost/token_iterator.hpp>

//#define VERBOSE_SERACH_MANAGER 1

using namespace std;

using izenelib::ir::indexmanager::TermDocFreqs;
using izenelib::ir::indexmanager::TermReader;

namespace sf1r{

QueryBuilder::QueryBuilder(
        const boost::shared_ptr<IndexManager> indexManager,
        const boost::shared_ptr<DocumentManager> documentManager,
        const boost::shared_ptr<IDManager> idManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        boost::unordered_map<std::string, PropertyConfig>& schemaMap,
        size_t filterCacheNum
)
    :indexManagerPtr_(indexManager)
    ,documentManagerPtr_(documentManager)
    ,idManagerPtr_(idManager)
    ,rankingManagerPtr_(rankingManager)
    ,schemaMap_(schemaMap)
{
    pIndexReader_ = indexManager->pIndexReader_;
    filterCache_.reset(new FilterCache(filterCacheNum));
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
        boost::shared_ptr<EWAHBoolArray<uint32_t> >& pDocIdSet)
{
    boost::shared_ptr<BitVector> pBitVector;
    unsigned int bitsNum = pIndexReader_->maxDoc() + 1;
    unsigned int wordsNum = bitsNum/(sizeof(uint32_t) * 8) + (bitsNum % (sizeof(uint32_t) * 8) == 0 ? 0 : 1);

    if (filtingList.size() == 1)
    {
        const QueryFiltering::FilteringType& filteringItem= filtingList[0];
        QueryFiltering::FilteringOperation filterOperation = filteringItem.operation_;
        const std::string& property = filteringItem.property_;
        const std::vector<PropertyValue>& filterParam = filteringItem.values_;
        if (!filterCache_->get(filteringItem, pDocIdSet))
        {
            pDocIdSet.reset(new EWAHBoolArray<uint32_t>());
            pBitVector.reset(new BitVector(bitsNum));
            indexManagerPtr_->makeRangeQuery(filterOperation, property, filterParam, pBitVector);
            //Compress bit vector
            pBitVector->compressed(*pDocIdSet);
            filterCache_->set(filteringItem, pDocIdSet);
        }
    }
    else
    {
        pDocIdSet.reset(new EWAHBoolArray<uint32_t>());
        pDocIdSet->addStreamOfEmptyWords(true, wordsNum);
        boost::shared_ptr<EWAHBoolArray<uint32_t> > pDocIdSet2;
        boost::shared_ptr<EWAHBoolArray<uint32_t> > pDocIdSet3;
        try
        {
            std::vector<QueryFiltering::FilteringType>::const_iterator iter = filtingList.begin();
            for (; iter != filtingList.end(); ++iter)
            {
                QueryFiltering::FilteringOperation filterOperation = iter->operation_;
                const std::string& property = iter->property_;
                const std::vector<PropertyValue>& filterParam = iter->values_;
                if (!filterCache_->get(*iter, pDocIdSet2))
                {
                    pDocIdSet2.reset(new EWAHBoolArray<uint32_t>());
                    pBitVector.reset(new BitVector(bitsNum));
                    indexManagerPtr_->makeRangeQuery(filterOperation, property, filterParam, pBitVector);
                    pBitVector->compressed(*pDocIdSet2);
                    filterCache_->set(*iter, pDocIdSet2);
                }
                pDocIdSet3.reset(new EWAHBoolArray<uint32_t>());
                if (iter->logic_ == QueryFiltering::OR)
                {
                    (*pDocIdSet).rawlogicalor(*pDocIdSet2, *pDocIdSet3);
                }
                else // if (iter->logic_ == QueryFiltering::AND)
                {
                    (*pDocIdSet).rawlogicaland(*pDocIdSet2, *pDocIdSet3);
                }
                (*pDocIdSet).swap(*pDocIdSet3);
            }
        }
        catch (std::exception& e)
        {
        }
    }
}

WANDDocumentIterator* QueryBuilder::prepare_wand_dociterator(
        SearchKeywordOperation& actionOperation,
        collectionid_t colID,
        const property_weight_map& propertyWeightMap,
        const std::vector<std::string>& properties,
        const std::vector<unsigned int>& propertyIds,
        bool readPositions,
        const std::vector<std::map<termid_t, unsigned> >& termIndexMaps
)
{
    size_t size_of_properties = propertyIds.size();

    WANDDocumentIterator* pWandScorer = new WANDDocumentIterator(
                                           propertyWeightMap,
                                           propertyIds,
                                           properties);
    if (pIndexReader_->isDirty())
    {
        pIndexReader_ = indexManagerPtr_->getIndexReader();
    }
    try{
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
    }catch(std::exception& e){
        delete pWandScorer;
        throw std::runtime_error("Failed to prepare wanddociterator");
        return NULL;
    }
}

void QueryBuilder::prepare_for_wand_property_(
        WANDDocumentIterator* pWandScorer,
        size_t & success_properties,
        SearchKeywordOperation& actionOperation,
        collectionid_t colID,
        const std::string& property,
        unsigned int propertyId,
        bool readPositions,
        const std::map<termid_t, unsigned>& termIndexMapInProperty
)
{
    typedef std::map<termid_t, unsigned>::const_iterator const_iter;
    typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator iterator;
    iterator found = schemaMap_.find(property);
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
        SearchKeywordOperation& actionOperation,
        collectionid_t colID,
        const property_weight_map& propertyWeightMap,
        const std::vector<std::string>& properties,
        const std::vector<unsigned int>& propertyIds,
        bool readPositions,
        const std::vector<std::map<termid_t, unsigned> >& termIndexMaps
)
{
    size_t size_of_properties = propertyIds.size();

    MultiPropertyScorer* docIterPtr = new MultiPropertyScorer(propertyWeightMap, propertyIds);
    if (pIndexReader_->isDirty())
    {
        pIndexReader_ = indexManagerPtr_->getIndexReader();
    }
    try{
        size_t success_properties = 0;
        typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator schema_iterator;
	
        for (size_t i = 0; i < size_of_properties; i++)
        {
            schema_iterator found = schemaMap_.find(properties[i]);
            if (found == schemaMap_.end())
                continue;
            if(found->second.subProperties_.empty())
                prepare_for_property_(
                    docIterPtr,
                    success_properties,
                    actionOperation,
                    colID,
                    found->second,
                    readPositions,
                    termIndexMaps[i]
                );
            else
                prepare_for_virtual_property_(
                    docIterPtr,
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
            return docIterPtr;
        else
            delete docIterPtr;

        return NULL;
    }catch(std::exception& e){
        delete docIterPtr;
        throw std::runtime_error("Failed to prepare dociterator");
        return NULL;
    }
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
    TermReader* pTermReader = NULL;
    if (!isNumericFilter)
    {
        pTermReader = pIndexReader_->getTermReader(colID);
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
            boost::shared_ptr<EWAHBoolArray<uint32_t> > pDocIdSet;
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
                if(!filterCache_->get(filteringRule, pDocIdSet))
                {
                    pDocIdSet.reset(new EWAHBoolArray<uint32_t>());
                    pBitVector.reset(new BitVector(pIndexReader_->maxDoc() + 1));

                    indexManagerPtr_->getDocsByNumericValue(colID, property, value, *pBitVector);
                    pBitVector->compressed(*pDocIdSet);
                    filterCache_->set(filteringRule, pDocIdSet);
                }
                TermDocFreqs* pTermDocReader = new BitMapIterator( pDocIdSet);
                termDocReaders[termId].push_back(pTermDocReader);
            }
        }
     }
     if (pTermReader)
        delete pTermReader;
#endif
}

void QueryBuilder::prepare_for_property_(
        MultiPropertyScorer* pScorer,
        size_t & success_properties,
        SearchKeywordOperation& actionOperation,
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

    for (size_t i = 0; i < indexPropertySize; ++i )
    {
        propertyRankers[i]->setupStats(rankQueryProperties[i]);
    }
}

void QueryBuilder::prepare_for_virtual_property_(
        MultiPropertyScorer* pScorer,
        size_t & success_properties,
        SearchKeywordOperation& actionOperation,
        collectionid_t colID,
        const PropertyConfig& properyConfig, //virtual property config
        bool readPositions,
        const std::map<termid_t, unsigned>& termIndexMapInProperty,
        const property_weight_map& propertyWeightMap
)
{
    QueryTreePtr queryTree;
    if (! actionOperation.getQueryTree(properyConfig.getName(), queryTree) ) return;

    typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator schema_iterator;
    DocumentIterator* pIter = NULL;
    std::map<termid_t, VirtualPropertyTermDocumentIterator* > virtualTermIters;
    bool hasVirtualIterBuilt = false;

    std::vector<pair<termid_t, string> > termList;
    actionOperation.getQueryTermInfoList(properyConfig.getName(), termList);
    std::sort(termList.begin(), termList.end());

    for(unsigned j = 0; j < properyConfig.subProperties_.size();++j)
    {
        schema_iterator p = schemaMap_.find(properyConfig.subProperties_[j]);
        if(p != schemaMap_.end())
        {
            std::map<termid_t, std::vector<TermDocFreqs*> > termDocReaders;
            bool isNumericFilter = false;
            prefetch_term_doc_readers_(termList,colID,p->second,readPositions,isNumericFilter,termDocReaders);
            if(pIter) hasVirtualIterBuilt  = true;
            do_prepare_for_property_(
                queryTree,
                colID,
                p->second.getName(),
                p->second.getPropertyId(),
                p->second.getType(),
                isNumericFilter,
                readPositions,
                termIndexMapInProperty,
                pIter,
                virtualTermIters,
                termDocReaders,
                actionOperation.hasUnigramProperty_,
                actionOperation.isUnigramSearchMode_,
                properyConfig.getName(),
                hasVirtualIterBuilt
            );
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
    if(pIter)
    {
        size_t indexSubPropertySize = properyConfig.subProperties_.size();
        std::vector<propertyid_t> indexSubPropertyIdList(indexSubPropertySize);
        std::transform(
            properyConfig.subProperties_.begin(),
            properyConfig.subProperties_.end(),
            indexSubPropertyIdList.begin(),
            boost::bind(&QueryBuilder::getPropertyIdByName, this, _1)
        );
        VirtualPropertyScorer* pVirtualScorer = new VirtualPropertyScorer(propertyWeightMap, indexSubPropertyIdList);
        pVirtualScorer->add(pIter);

        sf1r::TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;
        // references for property term info
        const property_term_info_map& propertyTermInfoMap =
                actionOperation.getPropertyTermInfoMap();

        DocumentFrequencyInProperties dfmap;
        CollectionTermFrequencyInProperties ctfmap;
        MaxTermFrequencyInProperties maxtfmap;
        rankingManagerPtr_->createPropertyRankers(pTextRankingType, indexSubPropertySize, pVirtualScorer->propertyRankers_);

        bool readTermPosition = pVirtualScorer->propertyRankers_[0]->requireTermPosition();
        pIter->df_cmtf(dfmap, ctfmap, maxtfmap);	
        post_prepare_ranker_(
            properyConfig.getName(),
            properyConfig.subProperties_,
            indexSubPropertySize,
            propertyTermInfoMap,
            dfmap,
            ctfmap,
            maxtfmap,
            readTermPosition,
            pVirtualScorer->rankQueryProperties_,
            pVirtualScorer->propertyRankers_);
        pScorer->add(properyConfig.getPropertyId(), pVirtualScorer);
        ++success_properties;
    }
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
        bool hasVirtualIterBuilt,
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
        try{
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
                    else
                        pDocIterator->add(pVirtualTermDocIter);
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
                        else
                            pDocIterator->add(pVirtualTermDocIter);
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
        }catch(std::exception& e){
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

        QueryTreePtr childQueryTree = *(queryTree->children_.begin());
        termid_t keywordId = childQueryTree->keywordId_;
        if (NULL == pDocIterator)
            ///only NOT is not permitted
            return false;
        unsigned termIndex = izenelib::util::getOr(
                                 termIndexMapInProperty,
                                 keywordId,
                                 (std::numeric_limits<unsigned>::max) ()
                             );
        TermDocumentIterator* pIterator =
            new TermDocumentIterator(keywordId,
                                     colID,
                                     pIndexReader_,
                                     property,
                                     propertyId,
                                     termIndex,
                                     readPositions);
        try{
#if PREFETCH_TERMID
        std::map<termid_t, std::vector<TermDocFreqs*> >::iterator constIt
        = termDocReaders.find(keywordId);
        if (constIt != termDocReaders.end() && !constIt->second.empty() )
        {
            pIterator->set(constIt->second.back());

            if(virtualProperty.empty())
            {
                ///General keyword search
                pIterator->setNot(true);
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
                    pVirtualTermDocIter->setNot(true);
                    pVirtualTermDocIter->add(pIterator);
                    virtualTermIters[keywordId] = pVirtualTermDocIter;
                    if(!hasVirtualIterBuilt)
                        pDocIterator->add(pVirtualTermDocIter);
                }
            }
            constIt->second.pop_back();
        }
        else
#endif
        {
            if (pIterator->accept())
            {
                if(virtualProperty.empty())
                {
                    ///General keyword search
                    pIterator->setNot(true);
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
                        pVirtualTermDocIter->setNot(true);
                        pVirtualTermDocIter->add(pIterator);
                        virtualTermIters[keywordId] = pVirtualTermDocIter;
                        pDocIterator->add(pVirtualTermDocIter);
                    }
                }
            }
            else
                delete pIterator;
        }
        }catch(std::exception& e)
        {
            delete pIterator;
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
        typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator iterator;
        iterator found = schemaMap_.find(unigramProperty);
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
        bool ret = false;
        try{
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
                      hasVirtualIterBuilt,
                      1
                  );
            if (virtualProperty.empty()&&!ret)
            {
                delete pIterator;
                return false;
            }
        }
        if (! static_cast<ANDDocumentIterator*>(pIterator)->empty())
        {
            if(!virtualProperty.empty())
            {
                ///And query on virtual property
                if(!hasVirtualIterBuilt)
                {
                    if(NULL == pDocIterator)
                        pDocIterator = pIterator;
                    else
                        pDocIterator->add(pIterator);
                }
                else
                {
                    delete pIterator;
                    return false;
                }
            }
            else
                ///General AND query
                if (NULL == pDocIterator)
                    pDocIterator = pIterator;
                else
                    pDocIterator->add(pIterator);
       	}
        else
            delete pIterator;
        }catch(std::exception& e){
            delete pIterator;
            return false;
        }
        break;
    } // end - QueryTree::AND
    case QueryTree::OR:
    {
#ifdef VERBOSE_SERACH_MANAGER
        cout<<"OR query "<<property<<endl;
#endif
        DocumentIterator* pIterator = new ORDocumentIterator();
        bool ret = false;
        try{
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
                       hasVirtualIterBuilt,
                       0
                   );
        }
        if (!ret)
        {
            delete pIterator;
            return false;
        }
        if (! static_cast<ORDocumentIterator*>(pIterator)->empty()) {
            if(!virtualProperty.empty())
            {
                if(!hasVirtualIterBuilt)
                {
                    if(NULL == pDocIterator)
                        pDocIterator = pIterator;
                    else
                        pDocIterator->add(pIterator);
                }
                else
                {
                    delete pIterator;
                    return false;
                }
            }
            else
                if (NULL == pDocIterator)
                    pDocIterator = pIterator;
                else
                    pDocIterator->add(pIterator);
        }
        else
            delete pIterator;
        }catch(std::exception& e)            {
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
        try{
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
                      hasVirtualIterBuilt
                  );
            if (virtualProperty.empty()&&!ret)
            {
                delete pIterator;
                ///return false;
                return true;
            }
        }
        if (! static_cast<PersonalSearchDocumentIterator*>(pIterator)->empty()) {
            if(!virtualProperty.empty())
            {
                if(!hasVirtualIterBuilt)
                {
                    if(NULL == pDocIterator)
                        pDocIterator = pIterator;
                    else
                        pDocIterator->add(pIterator);
                }
                else
                {
                    delete pIterator;
                    return false;
                }
            }
            else
                if (NULL == pDocIterator)
                    pDocIterator = pIterator;
                else
                    pDocIterator->add(pIterator);
        }
        else
            delete pIterator;
        }catch(std::exception& e){
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
        try{
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
                      virtualProperty,
                      hasVirtualIterBuilt
                  );
            if (!ret)
            {
                delete pIterator;
                ///return false;
                return true;
            }
        }
        if (! static_cast<PersonalSearchDocumentIterator*>(pIterator)->empty()) {
            if(!virtualProperty.empty())
            {
                if(!hasVirtualIterBuilt)
                {
                    if(NULL == pDocIterator)
                        pDocIterator = pIterator;
                    else
                        pDocIterator->add(pIterator);
                }
                else
                {
                    delete pIterator;
                    return false;
                }
            }
            else
                if (NULL == pDocIterator)
                    pDocIterator = pIterator;
                else
                    pDocIterator->add(pIterator);
        }
        else
            delete pIterator;
        }catch(std::exception& e){
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
        typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator iterator;
        iterator found = schemaMap_.find(unigramProperty);
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
    typedef boost::unordered_map<std::string, PropertyConfig>::const_iterator
    iterator;
    iterator found = schemaMap_.find(name);
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

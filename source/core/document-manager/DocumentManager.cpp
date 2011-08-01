///
/// @file   DocumentManager.cpp
/// @brief  Manages properties and rawtexts
/// @date   2009-10-21 12:16:36
/// @author Deepesh Shrestha, Peiseng Wang,
///

#include <common/SFLogger.h>
#include "DocumentManager.h"
#include "DocContainer.h"
#include "highlighter/Highlighter.h"
#include "snippet-generation-submanager/SnippetGeneratorSubManager.h"
#include "text-summarization-submanager/TextSummarizationSubManager.h"
#include <configuration-manager/ConfigurationTool.h>
#include <util/profiler/ProfilerGroup.h>
#include <license-manager/LicenseManager.h>

#include <fstream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/archive_exception.hpp>
#include <protect/RestrictMacro.h>


namespace sf1r
{

const std::string DocumentManager::ACL_FILE = "ACLTable";
const std::string DocumentManager::PROPERTY_LENGTH_FILE = "PropertyLengthDb.xml";
const std::string DocumentManager::PROPERTY_BLOCK_SUFFIX = ".blocks";

ilplib::langid::Factory* DocumentManager::langIdFactory = ilplib::langid::Factory::instance();

DocumentManager::DocumentManager(
    const std::string& path,
    const std::set<PropertyConfig, PropertyComp>& schema,
    const izenelib::util::UString::EncodingType encodingType,
    size_t documentCacheNum
)
    :path_(path)
    ,documentCache_(100)
    ,schema_(schema)
    ,encodingType_(encodingType)
    ,propertyLengthDb_()
    ,propertyIdMapper_()
    ,propertyAliasMap_()
    ,displayLengthMap_()
    ,maxSnippetLength_(200)
    ,threadPool_(20)
{
    propertyValueTable_ = new DocContainer(path);
    propertyValueTable_->open();
    buildPropertyIdMapper_();
    restorePropertyLengthDb_();
    //aclTable_.open();
    snippetGenerator_ = new SnippetGeneratorSubManager;
    highlighter_ = new Highlighter;

    langIdAnalyzer_ = langIdFactory->createAnalyzer();
    langIdKnowledge_ = langIdFactory->createKnowledge();
}

DocumentManager::~DocumentManager()
{
    if(propertyValueTable_) delete propertyValueTable_;
    if(snippetGenerator_) delete snippetGenerator_;
    if(highlighter_) delete highlighter_;
    if (langIdKnowledge_)  delete langIdKnowledge_;
    if (langIdAnalyzer_) delete langIdAnalyzer_;
}

bool DocumentManager::flush()
{
    propertyValueTable_->flush();
    return savePropertyLengthDb_();
}

void DocumentManager::setLangId(std::string& langIdDbPath)
{
    string encodingPath = langIdDbPath + "/model/encoding.bin";
    langIdKnowledge_->loadEncodingModel(encodingPath.c_str());
    // load language model for language identification or sentence tokenization
    string langPath = langIdDbPath + "/model/language.bin";
    langIdKnowledge_->loadLanguageModel(langPath.c_str());
    // set knowledge
    langIdAnalyzer_->setKnowledge(langIdKnowledge_);
}

bool DocumentManager::insertDocument(const Document& document)
{

    typedef Document::property_const_iterator iterator;
    for (iterator it = document.propertyBegin(), itEnd = document.propertyEnd(); it
            != itEnd; ++it)
    {
        const propertyid_t* pid = propertyIdMapper_.findIdByValue(it->first);
        if (!pid)
        {
            // not in config, skip
            continue;
        }

        const izenelib::util::UString* stringValue =
            get<izenelib::util::UString>(&it->second);
        if (stringValue)
        {
            if (propertyLengthDb_.size() <= *pid)
            {
                propertyLengthDb_.resize(*pid + 1);
            }
            propertyLengthDb_[*pid] += stringValue->length();
        }
    }

    if ( LicenseManager::continueIndex_ )
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE( document.getId(), LICENSE_MAX_DOC, 0 );
    }
    else
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE( document.getId(), LicenseManager::TRIAL_MAX_DOC, 0 );
    }

    if (!propertyValueTable_->insert(document.getId(), document))
    {
        return false;
    }

    return true;
}

bool DocumentManager::updateDocument(const Document& document)
{
    //if(acl_) aclTable_.update(document.getId(), document);

    if ( LicenseManager::continueIndex_ )
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE( document.getId(), LICENSE_MAX_DOC, 0 );
    }
    else
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE( document.getId(), LicenseManager::TRIAL_MAX_DOC, 0 );
    }

    if (propertyValueTable_->update(document.getId(), document))
    {
        documentCache_.del(document.getId());
        return true;
    }

    return false;
}

bool DocumentManager::updatePartialDocument(const Document& document)
{
    docid_t docId = document.getId();
    Document oldDoc;

    if (!getDocument(docId, oldDoc))
    {
        return false;
    }

    typedef Document::property_const_iterator iterator;

    for (iterator it = document.propertyBegin(), itEnd = document.propertyEnd(); it
            != itEnd; ++it)
    {
        const propertyid_t* pid = propertyIdMapper_.findIdByValue(it->first);
        if (!pid)
        {
            // not in config, skip
            continue;
        }

        if (it->first != "DOCID" && it->first != "DATE")
        {
            oldDoc.updateProperty(it->first, it->second);
        }
    }

    return updateDocument(oldDoc);
}

bool DocumentManager::isDeleted(docid_t docId)
{

    int64_t delIndicator=1;
    getPropertyValue(docId, "DOCID", delIndicator);

    if (delIndicator == -1)
        return true;

    return false;

}

bool DocumentManager::removeDocument(docid_t docId)
{
    //if(acl_) aclTable_.del(docId);
    return propertyValueTable_->del(docId);
    return true;
}

std::size_t DocumentManager::getTotalPropertyLength(const std::string& property) const
{

    boost::unordered_map< std::string, unsigned int>::const_iterator iter =
        propertyAliasMap_.find(property);

    if (iter != propertyAliasMap_.end() && iter->second
            < propertyLengthDb_.size() )
        return propertyLengthDb_[ iter->second ];
    else
    {
        const unsigned int* id = propertyIdMapper_.findIdByValue(property);
        if ( (id != 0) && (*id < propertyLengthDb_.size()))
            return propertyLengthDb_[*id];
    }

    return 0;
}

bool DocumentManager::getPropertyValue(
    docid_t docId,
    const std::string& propertyName, 
    PropertyValue& result
)
{

    Document doc;
    const std::string* realPropertyName = &propertyName;

    //restore alias property name to original
    boost::unordered_map< std::string, unsigned int>::iterator aIter =
        propertyAliasMap_.find(propertyName);
    if (aIter != propertyAliasMap_.end() )
    {
        realPropertyName = propertyIdMapper_.findValueById(aIter->second);
    }

    if (documentCache_.getValue(docId, doc) )
    {
        result = doc.property(*realPropertyName);
    }
    else
    {
        getDocument(docId, doc);
        documentCache_.insertValue(docId, doc);
        result = doc.property(*realPropertyName);
    }
//	if( getDocument(docId, doc) )
//		result = doc.property(*realPropertyName);
    return true;
}

bool DocumentManager::getDocument(
    docid_t docId, 
    Document& document
)
{
    CREATE_SCOPED_PROFILER ( getDocument, "DocumentManager", "DocumentManager::getDocument");
    return propertyValueTable_->get(docId, document);
}

bool DocumentManager::getDocumentAsync(docid_t docId)
{
    Document document;
    boost::detail::atomic_count finishedJobs(0);
    threadPool_.schedule(boost::bind(&DocumentManager::getDocument_impl, this, docId, document, &finishedJobs));

    return true;
}

bool DocumentManager::getDocument_impl(
    docid_t docId, 
    Document& document,
    boost::detail::atomic_count* finishedJobs
)
{
    if (documentCache_.getValue(docId, document) )
    {
        ++(*finishedJobs);
        return true;
    }
    if ( propertyValueTable_->get(docId, document) )
    {
        documentCache_.insertValue(docId, document);
        ++(*finishedJobs);
        return true;
    }
    ++(*finishedJobs);
    return false;
}

docid_t DocumentManager::getMaxDocId()
{
    return propertyValueTable_->getMaxDocId();
}

void DocumentManager::buildPropertyIdMapper_()
{
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap;
    config_tool::buildPropertyAliasMap(schema_, propertyAliasMap);

    typedef std::set<PropertyConfig, PropertyComp>::iterator prop_iterator;

    for (prop_iterator it = schema_.begin(), itEnd = schema_.end(); it != itEnd; ++it)
    {
        propertyid_t originalPropertyId(0), originalBlockId(0);

        bool hasSummary(it->getIsSummary() );
        bool hasSnippet(it->getIsSnippet() );

        if (hasSnippet || hasSummary)
        {
            originalBlockId = propertyIdMapper_.insert(it->getName()
                              + PROPERTY_BLOCK_SUFFIX);

            boost::unordered_map<std::string, unsigned int>::iterator dispIter;
            dispIter = displayLengthMap_.find(it->getName());
            if (dispIter == displayLengthMap_.end())
            {
                unsigned int dispLength = it->getDisplayLength();
                if ((dispLength < 50) || (dispLength > 250)) //50, 250 as min and max DisplayLength
                    dispLength = maxSnippetLength_;
                displayLengthMap_.insert(std::make_pair(it->getName(),
                                                        dispLength));
            }
        }
        originalPropertyId = propertyIdMapper_.insert(it->getName());

        // For alias property
        config_tool::PROPERTY_ALIAS_MAP_T::iterator aliasIter =
            propertyAliasMap.find(it->getName() );
        if (aliasIter != propertyAliasMap.end() )
        {

            for (std::vector<PropertyConfig>::iterator vecIter =
                        aliasIter->second.begin(); vecIter
                    != aliasIter->second.end(); vecIter++)
            {
                if (hasSnippet)
                {

                    propertyAliasMap_.insert(make_pair(vecIter->getName()
                                                       + PROPERTY_BLOCK_SUFFIX, originalBlockId) );

                }
                propertyAliasMap_.insert(make_pair(vecIter->getName(),
                                                   originalPropertyId) ) ;
            }
        }
    }
} // end - buildPropertyIdMapper_()

bool DocumentManager::savePropertyLengthDb_() const
{
    try
    {
        const std::string kDbPath = path_ + PROPERTY_LENGTH_FILE;
        std::ofstream ofs(kDbPath.c_str());
        if (ofs)
        {
            boost::archive::xml_oarchive oa(ofs);
            oa << boost::serialization::make_nvp(
                "PropertyLength", propertyLengthDb_
            );
        }

        return ofs;
    }
    catch (boost::archive::archive_exception& e)
    {
        sflog->error(SFL_IDX, 50102, e.what());
        return false;
    }
}

bool DocumentManager::restorePropertyLengthDb_()
{
    try
    {
        const std::string kDbPath = path_ + PROPERTY_LENGTH_FILE;
        std::ifstream ifs(kDbPath.c_str());
        if (ifs)
        {
            boost::archive::xml_iarchive ia(ifs);
            ia >> boost::serialization::make_nvp(
                "PropertyLength", propertyLengthDb_
            );
        }
        return ifs;
    }
    catch (boost::archive::archive_exception& e)
    {
        sflog->error(SFL_IDX, 50103, e.what());
        propertyLengthDb_.clear();
        return false;
    }
}

bool DocumentManager::getRawTextOfDocuments(
    const std::vector<docid_t>& docIdList, const string& propertyName,
    const bool summaryOn, const unsigned int summaryNum,
    const unsigned int option,
    const std::vector<izenelib::util::UString>& queryTerms,
    std::vector<izenelib::util::UString>& outSnippetList,
    std::vector<izenelib::util::UString>& outRawSummaryList,
    std::vector<izenelib::util::UString>& outFullTextList)
{
    try
    {
        std::vector<izenelib::util::UString> snippetList;
        std::vector<izenelib::util::UString> rawSummaryList;
        std::vector<izenelib::util::UString> fullTextList;

        izenelib::util::UString rawText; // raw text
        izenelib::util::UString result; // output variable to store return value

        typedef std::vector<docid_t>::const_iterator doc_iterator;
        for (doc_iterator docIt = docIdList.begin(), docItEnd = docIdList.end(); docIt
                != docItEnd; ++docIt)
        {
            result.clear();
            if (!getPropertyValue(*docIt, propertyName, rawText))
                return false;

            fullTextList.push_back(rawText);

            std::string sentenceProperty = propertyName + PROPERTY_BLOCK_SUFFIX;
            std::vector<CharacterOffset> sentenceOffsets;
            if (!getPropertyValue(*docIt, sentenceProperty, sentenceOffsets))
                return false;

            maxSnippetLength_ = getDisplayLength_(propertyName);
            processOptionForRawText(option, queryTerms, rawText,
                                    sentenceOffsets, result);

            if (result.size()> 0 )
                snippetList.push_back(result);
            else
                snippetList.push_back(rawText);

            //process only if summary is ON
            unsigned int numSentences = summaryNum <= 0 ? 1 : summaryNum;
            if (summaryOn)
            {
                izenelib::util::UString summary;
                getSummary(rawText, sentenceOffsets, numSentences,
                           option, queryTerms, summary);

                rawSummaryList.push_back(summary);
            }
        }
        outSnippetList.swap(snippetList);
        outFullTextList.swap(fullTextList);
        if (summaryOn)
            outRawSummaryList.swap(rawSummaryList);

        return true;
    }
    catch (std::exception& e)
    {
        return false;
    }
}

bool DocumentManager::getDocumentsParallel(
    const std::vector<unsigned int>& ids,
    vector<Document>& docs
)
{
    docs.resize(ids.size() );
    boost::detail::atomic_count finishedJobs(0);
    for (size_t i=0; i<ids.size(); i++)
    {
        threadPool_.schedule(boost::bind(&DocumentManager::getDocument_impl, this, ids[i], docs[i], &finishedJobs));
    }
    threadPool_.wait(finishedJobs, ids.size());
    return true;
}

bool DocumentManager::getRawTextOfDocuments(
    const std::vector<docid_t>& docIdList, 
    const string& propertyName,
    const unsigned int option,
    const std::vector<izenelib::util::UString>& queryTerms,
    std::vector<izenelib::util::UString>& rawTextList,
    std::vector<izenelib::util::UString>& fullTextList)
{
    map<docid_t, int> doc_idx_map;
    unsigned int docListSize = docIdList.size();

    rawTextList.resize(docListSize);
    fullTextList.resize(docListSize);

    for (unsigned int i=0; i<docListSize; i++)
        doc_idx_map.insert(make_pair(docIdList[i], i) );

    vector<Document> docs;
    vector<unsigned int> ids;
    map<docid_t, int>::iterator it = doc_idx_map.begin();
    for (; it != doc_idx_map.end(); it++)
    {
        ids.push_back(it->first);
    }
    getDocumentsParallel(ids, docs);
    //make getRawTextOfOneDocument_(...) called in the order of docid
    //	map<docid_t, int>::iterator
    it = doc_idx_map.begin();
    for (; it != doc_idx_map.end(); it++)
    {
        if (getRawTextOfOneDocument_(it->first, propertyName, option,
                                     queryTerms, rawTextList[it->second], fullTextList[it->second])
                == false)
        {
            return false;
        }
    }
    return true;
}

bool DocumentManager::getRawTextOfOneDocument_(
    const docid_t docId,
    const string& propertyName, 
    const unsigned int option,
    const std::vector<izenelib::util::UString>& queryTerms,
    izenelib::util::UString& outSnippet, 
    izenelib::util::UString& rawText
)
{

    if (!getPropertyValue(docId, propertyName, rawText))
    {
        //	return false;
    }
    if (rawText.empty())
    {
        sflog->error(SFL_SRCH, 50105, propertyName.c_str());
        return true;
    }

    std::vector<CharacterOffset> sentenceOffsets;
    if (option > 1)
    {
        if (!getPropertyValue(docId, propertyName + PROPERTY_BLOCK_SUFFIX,
                              sentenceOffsets))
            return false;
    }

    maxSnippetLength_ = getDisplayLength_(propertyName);
    processOptionForRawText(option, queryTerms, rawText, sentenceOffsets,
                            outSnippet);

    //put raw text to outSnippet if it is empty
    if (outSnippet.size() <= 0)
        outSnippet = rawText;

    return true;

}

bool DocumentManager::processOptionForRawText(
    const unsigned int option,
    const std::vector<izenelib::util::UString>& queryTerms,
    const izenelib::util::UString& rawText,
    const std::vector<CharacterOffset>& sentenceOffsets,
    izenelib::util::UString& result
)
{
    switch (option)
    {
    case X_RAWTEXT:
        result = rawText;
        break;

    case O_RAWTEXT:
        highlighter_->getHighlightedText(rawText, queryTerms, encodingType_,
                                        result);
        break;

    case X_SNIPPET:
        snippetGenerator_->getSnippet(rawText, sentenceOffsets, queryTerms,
                                     maxSnippetLength_, false, encodingType_, result);
        break;

    case O_SNIPPET:
        snippetGenerator_->getSnippet(rawText, sentenceOffsets, queryTerms,
                                     maxSnippetLength_, true, encodingType_, result);
        break;
    }
    return true;
}

bool DocumentManager::getSummary(
    const izenelib::util::UString& rawText,
    const std::vector<CharacterOffset>& sentenceOffsets,
    unsigned int numSentences, 
    const unsigned int option,
    const std::vector<izenelib::util::UString>& queryTerms,
    izenelib::util::UString& summary
)
{
    uint32_t offsetIndex = 1;
    uint32_t summaryCount = 0;
    uint32_t indexedSummaryCount = 0;

    //if stored summaries is less than the requested summary number
    if (numSentences > sentenceOffsets[indexedSummaryCount])
        numSentences = sentenceOffsets[indexedSummaryCount];

    while ( ( (offsetIndex + 1) < ((sentenceOffsets[indexedSummaryCount]*2)+1) )
            && (summaryCount < numSentences))
    {
        uint32_t length = sentenceOffsets[offsetIndex + 1]
                          - sentenceOffsets[offsetIndex];
        izenelib::util::UString result;
        izenelib::util::UString sentence;
        rawText.substr(sentence, sentenceOffsets[offsetIndex], length);
        offsetIndex += 2;
        switch (option)
        {
        case X_RAWTEXT: //don't highlight
        case X_SNIPPET:
            result = sentence;
            break;
        case O_RAWTEXT: //Do highlight
        case O_SNIPPET:
            if (highlighter_->getHighlightedText(sentence, queryTerms,
                                                encodingType_, result) == false)
            {
                result = sentence;
            }
            break;
        }
        if (result.length()> 0)
            summary += result;

        ++summaryCount;
    }
    return true;
}

unsigned int DocumentManager::getDisplayLength_(const string& propertyName)
{
    boost::unordered_map< std::string, unsigned int>::iterator dispIter =
        displayLengthMap_.find(propertyName);
    if (dispIter != displayLengthMap_.end())
        return dispIter->second;

    return 0;
}

bool DocumentManager::highlightText(
    const izenelib::util::UString& text,
    const std::vector<izenelib::util::UString> queryTerms, 
    izenelib::util::UString& outText
)
{
    izenelib::util::UString highlightedText;
    highlighter_->getHighlightedText(text, queryTerms, encodingType_,
                                    highlightedText);
    outText.swap(highlightedText);
    return true;
}

}


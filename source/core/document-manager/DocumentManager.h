///
/// @file sf1r/document-manager/DocumentManager.h
/// @date Created <2009-10-20 16:03:45>
/// @date Updated <2010-06-22 12:16:49>
/// @date Updated <2010-01-21 09:04:44>
/// @author Deepesh Shrestha, Peisheng Wang
/// @history
/// -2009-11-16 Peisheng Wang
///    -Make documentManager use theadsafe SDB(b+ tree)
///    -Improve getRawTextOfDocuments
///

#ifndef SF1V5_DOCUMENT_MANAGER_DOCUMENT_MANAGER_H
#define SF1V5_DOCUMENT_MANAGER_DOCUMENT_MANAGER_H

#include <util/profiler/ProfilerGroup.h>
#include <common/PropertyKey.h>
#include <common/PropertyValue.h>

#include "Document.h"

#include <configuration-manager/PropertyConfig.h>
#include <util/IdMapper.h>

#include <ir/id_manager/IDManager.h>

#include <am/sequence_file/ssfr.h>

#include <util/ustring/UString.h>

#include <langid/langid.h>

#include <string>
#include <set>
#include <vector>
#include <deque>

#include <boost/thread.hpp>
#include <boost/threadpool.hpp>
#include <boost/dynamic_bitset.hpp>

namespace sf1r
{

enum DMOptionFlag
{
    //X means highlighting is NOT requested
    //O means highlighting requested
    X_RAWTEXT = 0,
    O_RAWTEXT = 1,
    X_SNIPPET = 2,
    O_SNIPPET = 3,
};

class SnippetGeneratorSubManager;
class Highlighter;
class DocContainer;

class DocumentManager
{
    typedef uint32_t CharacterOffset;
    typedef uint32_t DelFilterBlockType;
    typedef boost::dynamic_bitset<DelFilterBlockType> DelFilterType;
public:
    typedef Document DocumentType;
    /**
     * @brief initializes manager with @a path as working directory.
     *
     * @param path working directory, all disk files should reside here
     * @param propertyConfigs property specification read from config file.
     */
    DocumentManager(
        const std::string& path,
        const std::set<PropertyConfig, PropertyComp>& schema,
        izenelib::util::UString::EncodingType encondingType,
        size_t documentCacheNum = 20000
    );
    /**
     * @brief a destructor
     */
    ~DocumentManager();

    /**
     * @brief flush all data to disk files
     * @return @c true if all data are flushed to disk files successfully, @c
     *         @c false other wise.
     */
    bool flush();

    /*
    * @brief create language identifier instance for TextSummarizationSubManager
    */
    void setLangId(std::string& langIdDbPath);


    ilplib::langid::Analyzer* getLangId()
    {
        return langIdAnalyzer_;
    }

    /**
     * @brief inserts a new document. Has no effect if a document with the same
     * id exists
     * @exception DocumentManagerException cannot read data from \a document
     * @return \c true if the document has been inserted successfully, \c false
     *         if the document id exists, or failed to insert.
     */
    bool insertDocument(const Document& document);

    /**
     * @brief updates an existing document.
     * @return \c true if document has been updated successfully, \c false
     *         otherwise.
     * @warn if some properties exist in old document but not in the new one,
     *       these properties will not be removed in current implementation.
     * @warn the property length is not updated in current implementation.
     * @warn current implementation will inserted the document if the id does
     *       not exist.
     */
    bool updateDocument(const Document& document);

    /**
     * @brief updates an existing document.
     * @return \c true if document has been updated successfully, \c false
     *         otherwise.
     * @warn the new properties will be added in the old document.
     * @warn the property length is not updated in current implementation.
     * @warn current implementation will inserted the document if the id does
     *       not exist.
     */
    bool updatePartialDocument(const Document& document);

    /**
     * @brief deletes a document by given id.
     * @return \c true if document has been deleted successfully, \c false
     *         otherwise.
     */
    bool removeDocument(docid_t docId);

    /**
     * @brief checks if document is deleted by given id.
     * @return \c true if document is already deleted \c false
     *         otherwise.
     */
    bool isDeleted(docid_t docId);

    /**
     * @brief gets one document by id
     * @param docId document id
     * @param[out] document returned document, it will not be touched if
     *                      returning \c false
     * @return \c true if the document existed, and \a document has set to the
     *         correct value, \c false otherwise.
     */
    bool getDocument(
        docid_t docId, 
        Document& document
    );

    bool getDocumentAsync(docid_t docId);

    bool getDocument_impl(
        docid_t docId, 
        Document& document, 
        boost::detail::atomic_count* finishedJobs
    );

    /**
     * @brief gets total \c propertyLength_[i] in \c propertyDb_[i]
     * @param property property name
     * @return \c size if the property existed in either \c propertyAliasMap_
     *          or in the propertyIDMapper_
     */
    std::size_t getTotalPropertyLength(const std::string& property) const;

    /**
     * @brief get property value
     * @param docId document id
     * @param propertyName property name
     * @param[out] result fetched value if return \c true.
     * @return \c true if property has been found
     */
    bool getPropertyValue(
        docid_t docId, 
        const std::string& propertyName,
        PropertyValue& result
    );

    /**
     * @brief get property value and convert to type \c T
     * @return \c true if property has been found and has the type \c T
     */
    template<typename T> bool getPropertyValue(docid_t docId,
            const std::string& propertyName, T& result)
    {
        PropertyValue value;
        if (getPropertyValue(docId, propertyName, value))
        {
            T* castValue = get<T>(&value);
            if (castValue)
            {
                using std::swap;
                swap(result, *castValue);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief highlight text
     * @param text text fragment in which query terms are highlighted
     * @param queryTerms are terms of query
     * @param outText is the output text
     * @return true if highlight is successful else false
     */
    bool highlightText(
        const izenelib::util::UString& text,
        const std::vector<izenelib::util::UString> queryTerms,
        izenelib::util::UString& outText);

    /**
     * @brief fetches summary, snippet and highlight with options.
     *      TODO: Further refactoring of this method is required
     * @param docIdList result doc Id list for which the results are to be
     *                  fetched
     * @param propertyName property for which results are fetched
     * @param bSummary if true summary is processed
     * @param option for highlighted snippet, summary, rawText or otherwise
     * @param queryTermString holds original and analyzed query terms
     * @param[out] outSnippetList holds snippet result for given docIdList
     * @param[out] outRawSummaryList holds summary result for given docIdList
     * @param[out] outFullTextList holds raw Text result for given docIdList
     * @return true if results are produced according to the options
     */

    bool getRawTextOfDocuments(
        const std::vector<docid_t>& docIdList,
        const string& propertyName,
        const bool summaryOn,
        const unsigned int summaryNum,
        const unsigned int option,
        const std::vector<izenelib::util::UString>& queryTermString,
        std::vector<izenelib::util::UString>& outSnippetList,
        std::vector<izenelib::util::UString>& outRawSummaryList,
        std::vector<izenelib::util::UString>& outFullTextList
    );

    /**
     * @brief handles snippet and highlight with options: Method needs
     *        to be splitted according to the design and understanding with the
     *        Presentation Manager
     * @param docIdList result docId list for which the results are to be
     *                  fetched
     * @param propertyName names of the property for which results are to be
     *                     generated
     * @param option option int for summary, snippet, rawText and hihglighting
     * @param queryTerms contains original query, tokenized query terms of original
     *                   query, analyzed query for highlighting and snippet.
     * @param[out] outSnippetList returns snippets for documents in @a docIdList, if
     *                            snippet option is disabled returns corresponding
     *                            rawtext.
     * @param[out] outFullTextList returns rawtext for documents in @a docIdList.
     *                             Possible duplication of rawtext in \c outSnippetList,
     *                             and outFullTextList when snippet option is disabled,
     *                             however is mandatory for caching the rawtext in
     *                             presentation manager for mining operation and retrieval.
     * @return \c true if fetching rawtext for given property is successful
     */
    bool getRawTextOfDocuments(
        const std::vector<docid_t>& docIdList,
        const string& propertyName,
        const unsigned int option,
        const std::vector<izenelib::util::UString>& queryTerms,
        std::vector<izenelib::util::UString>& outSnippetList,
        std::vector<izenelib::util::UString>& outFullTextList
    );

    /**
     * @brief  returns the maximum doc Id value managed by document manager. The docId
     *         is expected to be ordered when inserted. Doc Id is generated by ID manager
     *         which is mapped to the DOCID in SCD also called SCD_DOCID. Interface used
     *         by IndexBuilder for correctly processing I/U/D/R SCDs and indexing.
     * @return  returns maximum docId value managed by document manager
     */
    docid_t getMaxDocId();
    
    bool getDeletedDocIdList(std::vector<docid_t>& docid_list);
    
    

private:
    bool loadDelFilter_();
    
    bool saveDelFilter_();
    
    /**
     * @brief builds property-id pair map for properties from configuration. Different
     *               map for alias is constructed in the same interface.
     */
    void buildPropertyIdMapper_();

    /**
     * @brief saves property length
     */
    bool savePropertyLengthDb_() const;

    /**
     * @brief restores property length
     */
    bool restorePropertyLengthDb_();

    /**
     * @brief gets rawtext for a single doc id
     * @param docId docId for which the results are to be fetched
     * @param propertyName names of the property for which results are to be
     *                     generated
     * @param option option int for summary, snippet, rawText and hihglighting
     * @param queryTerms contains original query, tokenized query terms of original
     *                   query, analyzed query for highlighting and snippet.
     * @param[out] outSnippetList returns snippets for documents in @a docIdList, if
     *                            snippet option is disabled returns corresponding
     *                            rawtext.
     * @param[out] outFullTextList returns rawtext for documents in @a docIdList.
     *                             Possible duplication of rawtext in \c outSnippetList,
     *                             and outFullTextList when snippet option is disabled,
     *                             however is mandatory for caching the rawtext in
     *                             presentation manager for mining operation and retrieval.
     * @return returns true if getting @a document is successful
     */
    bool getRawTextOfOneDocument_(
        const docid_t docId,
        const string& propertyName,
        const unsigned int option,
        const std::vector<izenelib::util::UString>& queryTerms,
        izenelib::util::UString& outSnippetList,
        izenelib::util::UString& outFullTextList
    );

    /**
     * @brief process options for summary, snippet and highlight for getRawText
     *        interfaces defined above.
     * @param option option int for summary, snippet, rawText and hihglighting
     * @param queryTerms contains original query, tokenized query terms of original
     *                   query, analyzed query for highlighting and snippet.
     * @param rawText rawtext for which snippet and highlighting might needed to be
     *                done depending upon option
     * @param sentenceOffsets offset pairs corresponding to rawtext for generating
     *                        sentence pairs for both summary and snippet
     * @param[out] result holds processed text, would either be highlighted text,
     *              highlighted snippet text, snippet text or rawtext itself.
     * @return returns true when option is processed correctly
     */
    bool processOptionForRawText(
        const unsigned int option,
        const std::vector<izenelib::util::UString>& queryTerms,
        const izenelib::util::UString& rawText,
        const std::vector<CharacterOffset>& sentenceOffsets,
        izenelib::util::UString& result);

    /**
     * @brief fetches summary sentences as requested by geRawTextOfDocument
     * @param sentenceOffsets offset pairs corresponding to rawtext for generating
     *                        sentence pairs for both summary and snippet. First
     *                        value holds number of summary sentences. Subsequent
     *                        celles holds start and end offset of sentences
     *                        corresponding to rawtext
     * @param numOfSentences  number of summary sentences as requested by getRaw
     *                        TextOfDocument
     * @param option option int for summary highlighting
     * @param queryTerms contains original query, tokenized query terms of original
     *                   query, analyzed query for highlighting and snippet.
     * @param[out] summary  holds summary sentences, either highlighted text or non
     *                      highlighted as stated by \c option
     * @return returns true when summary is generated.
     */
    bool getSummary(
        const izenelib::util::UString& rawText,
        const std::vector<CharacterOffset>& sentenceOffsets,
        unsigned int numSentences,
        const unsigned int option,
        const std::vector<izenelib::util::UString>& queryTerms,
        izenelib::util::UString& summary
    );

    unsigned int getDisplayLength_(const string& propertyName);

    bool getDocumentsParallel(
        const std::vector<unsigned int>& ids, 
        vector<Document>& docs
    );

private:
    /// @brief path for the index property file
    std::string path_;

    bool acl_;

    /// @brief SDB file that holds all corresponding property and files
    DocContainer* propertyValueTable_;
    
    /// @brief The delete flag filter
    DelFilterType delfilter_;
    
    boost::mutex delfilter_mutex_;

    /// @brief document cache holds the retrieved property values of document
    izenelib::cache::IzeneCache<docid_t, Document, izenelib::util::ReadWriteLock> documentCache_;

    /// @brief property specification from the configuration file
    std::set<PropertyConfig, PropertyComp> schema_;

    /// @brief encoding type used in system
    izenelib::util::UString::EncodingType encodingType_;

    /// @brief property length, \c propertyLength_[i] stores length for property
    /// which id is mapped to i though \c propertyIdMapper_
    std::vector<std::size_t> propertyLengthDb_;

    /// @brief maps property name to id for performance
    izenelib::util::IdMapper<std::string, propertyid_t> propertyIdMapper_;

    /// @brief maps property map for Alias
    boost::unordered_map< std::string, unsigned int> propertyAliasMap_;

    /// @brief maps property name to the display Length
    boost::unordered_map<std::string, unsigned int> displayLengthMap_;

    /// @brief display length from the configuration to limit the length of snippets
    unsigned int maxSnippetLength_;

    /// @brief used for calling snippet generation submanager to generate snippet
    SnippetGeneratorSubManager* snippetGenerator_;

    /// @brief used for highlighting the rawtext snippet
    Highlighter* highlighter_;

    boost::mutex mutex_;

    boost::threadpool::pool threadPool_;

    ilplib::langid::Analyzer* langIdAnalyzer_;

    ilplib::langid::Knowledge* langIdKnowledge_;

private:
    static const std::string INDEX_FILE;
    static const std::string ACL_FILE;
    static const std::string PROPERTY_LENGTH_FILE;
    static const std::string PROPERTY_BLOCK_SUFFIX;
    static unsigned int CACHE_SIZE;
    static ilplib::langid::Factory* langIdFactory;

};

} // end - namespace sf1r

#endif // SF1V5_DOCUMENT_MANAGER_DOCUMENT_MANAGER_H

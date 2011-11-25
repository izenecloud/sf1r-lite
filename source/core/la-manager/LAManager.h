/// @file LAManager.h
/// @brief header file of LAManager
/// @date 2008-07-04
/// @details
/// - Log
///     - 2009.07.09 Merged some interfaces into one by dohyun Yun.

#if !defined(_LA_MANAGER_)
#define _LA_MANAGER_

#include "AnalysisInformation.h"
#include "LAPool.h"

#include <la/LA.h>

#include <util/ustring/UString.h>

namespace la
{
class PlainDictionary;
}

namespace sf1r
{

///
/// @brief interface of LAManager
/// This class manages all kinds of offering term information.
/// When extracting terms from string, sysnonyms are also added behind of the term.
/// Only Lanaguage Extractor can offer the synonyms.
///
class LAManager
{
public:
    ///
    /// constructor.
    /// @param isMultiThreadEnv Whether the Entire running environment is multi-thread, default is true.
    ///
    LAManager( bool isMultiThreadEnv = true );

    ///
    /// destructor.
    ///
    ~LAManager();

    void loadStopDict( const std::string & path );

    template<typename IDManagerType>
    bool getTermIdList( IDManagerType * idm,
        const izenelib::util::UString & text,
        const AnalysisInfo& analysisInfo,
        TermIdList& termIdList,
        la::MultilangGranularity indexingLevel = la::FIELD_LEVEL)
    {
        la::LA * pLA = NULL;

        pLA = isMultiThreadEnv_ ? laPool_->popSearchLA( analysisInfo ) :
                laPool_->topSearchLA( analysisInfo );

        if(!pLA)
        {
            // std::cerr << "[LAManager] Error : Cannot get LA instance in " << __FILE__ << ":" << __LINE__ << std::endl;
            return false;
        }

        LAConfigUnit config;
        if(laPool_->getLAConfigUnit( analysisInfo.analyzerId_, config ))
        {
            // do not index synonym
            if (config.getAnalysis() == "multilang" )
                static_cast<la::MultiLanguageAnalyzer*>(pLA->getAnalyzer().get())->setExtractSynonym(false);
        }

        pLA->process( idm, text, termIdList, indexingLevel );

        if( isMultiThreadEnv_ )
            laPool_->pushSearchLA(analysisInfo, pLA);
        return true;
    }

    bool getTermList(
        const izenelib::util::UString & text,
        const AnalysisInfo& analysisInfo,
        la::TermList& termList );

    /// @brief  Gets ExpandedQuery with only default tokenizing.
    bool getExpandedQuery(
        const izenelib::util::UString& text,
        const AnalysisInfo& analysisInfo,
        bool isCaseSensitive,
        bool isSynonymInclude,
        izenelib::util::UString& expQuery );

    /// @brief Remove Stop Words in the termList
    /// @param termList input and output list
    /// @param stopDict stop word dictionary
    void removeStopwords(
        la::TermList & termList,
        shared_ptr<la::PlainDictionary>&  stopDict
    );
    
    la::LA* get_la(const AnalysisInfo& analysisInfo);

private:

    bool isMultiThreadEnv_; ///< Whether the Entire running environment is single-thread

    LAPool *laPool_;

    boost::shared_ptr<la::PlainDictionary> stopDict_;

};

} // namespace sf1r

#endif // _LA_MANAGER_

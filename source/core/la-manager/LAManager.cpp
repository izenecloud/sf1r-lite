/// @file LAManager.cpp
/// @brief source file of LAManager
/// @author JunHui
/// @date 2008-07-04
/// @details
/// - Log
///     - 2009.07.09 Merged some interfaces into one by dohyun Yun.

#include "LAManager.h"
#include <util/profiler/ProfilerGroup.h>
#include <list>
#include <sstream>

/*#ifdef SF1_TIME_CHECK
    #include "ProfilerGroup.h"
#endif*/

using namespace izenelib::util;
using namespace la;

namespace sf1r
{

    LAManager::LAManager( bool isMultiThreadEnv )
        : isMultiThreadEnv_( isMultiThreadEnv )
    {
    	laPool_ = LAPool::getInstance();
    }

    LAManager::~LAManager()
    {
    }

    void LAManager::loadStopDict( const std::string & path )
    {
        if( stopDict_.get() == NULL )
            stopDict_.reset( new PlainDictionary() );
        stopDict_->reloadDict( path.c_str() );
    }

    bool LAManager::getTermList(
            const izenelib::util::UString & text,
            const AnalysisInfo& analysisInfo,
            la::TermList& termList )
    {
        LA * pLA = NULL;

        pLA = isMultiThreadEnv_ ? laPool_->popSearchLA( analysisInfo ) :
                laPool_->topSearchLA( analysisInfo );

        if(!pLA)
        {
            // std::cerr << "[LAManager] Error : Cannot get LA instance in " << __FILE__ << ":" << __LINE__ << std::endl;
            return false;
        }

        pLA->process( text, termList );

        if( isMultiThreadEnv_ )
            laPool_->pushSearchLA(analysisInfo, pLA);

        return true;
    }

    bool LAManager::getExpandedQuery(
            const izenelib::util::UString& text,
            const AnalysisInfo& analysisInfo,
            bool isCaseSensitive,
            bool isSynonymInclude,
            izenelib::util::UString& expQuery )
    {
        LA * pLA = NULL;
        pLA = isMultiThreadEnv_ ? laPool_->popSearchLA( analysisInfo ) :
			laPool_->topSearchLA( analysisInfo );

        if(!pLA)
        {
            // Error
            //std::cerr << "[LAManager] Error : Cannot get LA instance in " << __FILE__ << ":" << __LINE__ << std::endl;
            return false;
        }

        TermList termList;

        LAConfigUnit config;
        if( laPool_->getLAConfigUnit( analysisInfo.analyzerId_, config ) )
        {
//            if( config.getAnalysis() == "korean" )
//                static_cast<NKoreanAnalyzer*>(pLA->getAnalyzer().get())->setSearchSynonym(isSynonymInclude);
        }

        /*if( isCaseSensitive )
        {*/
            pLA->process( text, termList );
        /*}
        else
        {
            UString temp = text;
            temp.toLowerString();
            pLA->process_search( temp, termList );
        }*/

        removeStopwords( termList, stopDict_ );

        /*for(TermList::iterator it = termList.begin(); it!=termList.end(); it++ ) {
            cout << "^^^^" << la::to_utf8(it->text_) << endl;
        }*/

         expQuery = toExpandedString( termList );

        //cout << "##########################" << la::to_utf8(expQuery) << endl;


        if( isMultiThreadEnv_ )
            laPool_->pushSearchLA(analysisInfo, pLA);

        return true;
    }

    void LAManager::removeStopwords(
            TermList & termList,
            shared_ptr<la::PlainDictionary>&  stopDict
            )
    {
        /// TODO: add stopDict support
//        if( stopDict.get() != NULL )
//            LA::removeStopwords( termList, stopDict );
    }
    
    la::LA* LAManager::get_la(const AnalysisInfo& analysisInfo)
    {
      LA * p_la = NULL;
      p_la = isMultiThreadEnv_ ? laPool_->popSearchLA( analysisInfo ) :
      laPool_->topSearchLA( analysisInfo );
      return p_la;
    }

} // namespace sf1r

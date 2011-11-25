#include <common/SFLogger.h>
#include "QMCommonFunc.h"
#include "QueryParser.h"

#include <fstream>

#ifdef QP_DETAIL_INFO
    #define QP_INFO ERROR
#else
    #define QP_INFO INFO
#endif

namespace sf1r {


    boost::shared_ptr<
        izenelib::am::rde_hash<izenelib::util::UString,bool>
        > QueryUtility::restrictTermDicPtr_;
    boost::shared_ptr<
        izenelib::am::rde_hash<termid_t,bool>
        > QueryUtility::restrictTermIdDicPtr_;

    std::string QueryUtility::dicPath_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> QueryUtility::idManager_;
    boost::shared_mutex QueryUtility::sharedMutex_;

    bool QueryUtility::reloadRestrictTermDictionary(void)
    {
        if (dicPath_.empty() || idManager_ == NULL )
        {
            sflog->warn(SFL_INIT, 110406);
            return false;
        }
        return buildRestrictTermDictionary(dicPath_, idManager_);
    }

    bool QueryUtility::buildRestrictTermDictionary(const std::string& dicPath,
            const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager)
    {
        // Write Lock
        boost::upgrade_lock<boost::shared_mutex> lock(sharedMutex_);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        {

            restrictTermDicPtr_.reset(new izenelib::am::rde_hash<izenelib::util::UString,bool>());
            restrictTermIdDicPtr_.reset(new izenelib::am::rde_hash<termid_t,bool>());

            // ---------------------------- [ Build restrictTermIdList ]
            // TODO : Currently Encoding type is following the encoding type of KMA.
            izenelib::util::UString::EncodingType encodingType = izenelib::util::UString::CP949;

            std::ifstream fpin( dicPath.c_str() );
            if ( !fpin.is_open() )
            {
                sflog->warn(SFL_INIT, 110401, dicPath.c_str());
                //std::cerr << "[QueryUtility] Warning : (Line " << __LINE__
                //    << ") : Restrict word ditionary File(" << dicPath << ") is not opened."
                //            << std::endl;
                return false;
            }

            while( fpin.good() )
            {
                termid_t termId;
                std::string restrictTerm;
                getline( fpin, restrictTerm);
                if (restrictTerm.empty() || restrictTerm[0] == ';')
                    continue;
                izenelib::util::UString restrictUTerm(restrictTerm, encodingType);
                idManager->getTermIdByTermString( restrictUTerm, termId );
                std::cerr << "[[[[[[[ Restrict keyword : ";restrictUTerm.displayStringValue(izenelib::util::UString::UTF_8, std::cerr); std::cerr << std::endl;
                sflog->info(SFL_INIT, 110402, restrictTerm.c_str());
                //cout << "REST Term : " << restrictTerm << " , " << termId << endl; // XXX
                //QueryUtility::restrictTermDicPtr_->insert( std::make_pair(restrictUTerm, true) );
                QueryUtility::restrictTermDicPtr_->insert(restrictUTerm, true);
                QueryUtility::restrictTermIdDicPtr_->insert(termId, true);
            }

            fpin.close();
        }

        // Set dicPath and idmanager if dictionary loading is successful.
        dicPath_ = dicPath;
        idManager_ = idManager;

        return true;

    } // end - buildRestrictTermDictionary()

    bool QueryUtility::isRestrictWord(const izenelib::util::UString& inputTerm)
    {
        boost::shared_lock<boost::shared_mutex> lock(sharedMutex_);
        if( !QueryUtility::restrictTermDicPtr_ ) return false;
        if ( QueryUtility::restrictTermDicPtr_->find( inputTerm ) != NULL )
        {
            std::string restrictKeyword;
            inputTerm.convertString(restrictKeyword, izenelib::util::UString::UTF_8); // XXX

            sflog->warn(SFL_SRCH, 110403, restrictKeyword.c_str());
            //cerr << "[QueryUtility] Warning : Restricted Keyword (" << restrictKeyword
            //        << ") occurs" << endl;
            return true;
        }
        return false;
    } // end - isRestrictWord()

    bool QueryUtility::isRestrictId(termid_t termId)
    {
        boost::shared_lock<boost::shared_mutex> lock(sharedMutex_);
        if ( QueryUtility::restrictTermIdDicPtr_->find( termId ) != NULL )
        {

            sflog->warn(SFL_SRCH, 110404, termId);
            //cerr << "[QueryUtility] Warning : Restricted termid (" << termId
            //        << ") occurs" << endl;
            return true;
        }
        return false;
    } // end - isRestrictId()

    void QueryUtility::getMergedUniqueTokens(
            const std::vector<izenelib::util::UString>& inputTokens,
            const izenelib::util::UString& rawString,
            boost::shared_ptr<LAManager> laManager,
            std::vector<izenelib::util::UString>& resultTokens,
            bool useOriginalQuery
            )
    {
        std::vector<izenelib::util::UString> tmpResultTokens;

        std::set<izenelib::util::UString> queryTermSet;
        useOriginalQuery = true;

        //insert original query string in its form
        if ( useOriginalQuery )
            queryTermSet.insert(rawString);

        { // get tokenized query terms form LAManager
            la::TermList termList;
            AnalysisInfo analysisInfo;
            laManager->getTermList(rawString, analysisInfo, termList );
            for (la::TermList::iterator p = termList.begin(); p != termList.end(); ++p)
            {
                QueryParser::removeEscapeChar(p->text_);
                queryTermSet.insert(p->text_);
            }
        }

        //insert input Tokens
        std::vector<izenelib::util::UString>::const_iterator termIter = inputTokens.begin();
        for (; termIter != inputTokens.end(); ++termIter)
            queryTermSet.insert(*termIter);

        //pack all in one vector with word-restriction processing
        std::set<izenelib::util::UString>::iterator iter = queryTermSet.begin();
        for (; iter != queryTermSet.end(); ++iter)
        {
            if ( QueryUtility::isRestrictWord( *iter ) )
                continue;
            tmpResultTokens.push_back(*iter);
        }

        resultTokens.swap(tmpResultTokens);
    } // end - getMergedUniqueTokens()

    void QueryUtility::getMergedUniqueTokens(
            const izenelib::util::UString& rawString,
            boost::shared_ptr<LAManager> laManager,
            std::vector<izenelib::util::UString>& resultTokens,
            bool useOriginalQuery
            )
    {
        QueryUtility::getMergedUniqueTokens(
                std::vector<izenelib::util::UString>(),
                rawString,
                laManager,
                resultTokens,
                useOriginalQuery
                );
    } // end - getMergedUniqueTokens()

} // end - namespace sf1r

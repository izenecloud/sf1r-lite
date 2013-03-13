/// @file   QMCommonFunc.h
/// @brief  Common functions which is related to  Query Manager.
/// @author Dohyun Yun
/// @date   2009.08.19
/// @details
/// - Log
///     - 2009.08.21 Refined source code by Dohyun Yun
///         - Added laManager parameter in buildQueryTree()
///

#ifndef _QMCOMMONFUNC_H_
#define _QMCOMMONFUNC_H_

#include "ActionItem.h"
#include <la-manager/LAManager.h>

#include <ir/id_manager/IDManager.h>
#include <am/3rdparty/rde_hash.h>

#include <la/dict/UpdatableDict.h>

namespace sf1r {

    ///
    /// @brief a class which is used to process restriction, query-tree building, etc.
    ///
    class QueryUtility : boost::noncopyable
    {
        private:
            QueryUtility() {};
            ~QueryUtility() {};

        private:
            ///
            /// @brief a dictionary variable which contains restrict term.
            ///
            static boost::shared_ptr<
                izenelib::am::rde_hash<izenelib::util::UString,bool>
                > restrictTermDicPtr_;

            ///
            /// @brief a dictionary variable which contains restrict term id.
            ///
            static boost::shared_ptr<
                izenelib::am::rde_hash<termid_t,bool>
                > restrictTermIdDicPtr_;

            ///
            /// @brief contains the path of restriction dictionary.
            ///
            static std::string dicPath_;

            ///
            /// @brief pointer of id manager which is used in the QueryUtility.
            ///
            static boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;

            ///
            /// @brief a mutex which is used while building restriction dictionary.
            ///
            static boost::shared_mutex sharedMutex_;

        public:

            static bool reloadRestrictTermDictionary(void);
            static bool buildRestrictTermDictionary(const std::string& dicPath,
                    const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager);

            static bool isRestrictWord(const izenelib::util::UString& inputTerm);
            static bool isRestrictId(termid_t termId);

            ///
            /// @brief this interface merges input tokens and the tokens of rawString into resultTokens by using lamanager.
            /// @details resultTokens will contain following lists. While inserting tokens, only unique terms can be remained.
            ///     - Whole string of raw String (if useOriginalQuery == true)
            ///     - tokens of raw string.
            ///     - inputTokens
            /// @param[inputTokens]         an input tokens to generate unique toknes.
            /// @param[rawString]           a string which is tokenized to raw tokens by using third parameter object (LA Manager)
            /// @param[laManager]           an object which is used to do default tokenize rawString.
            /// @param[resultTokens]        a result merged unique tokens of this interface.
            /// @param[useOriginalQuery]    a flag to add original query.
            ///
            static void getMergedUniqueTokens(
                    const std::vector<izenelib::util::UString>& inputTokens,
                    const izenelib::util::UString& rawString,
                    boost::shared_ptr<LAManager> laManager,
                    std::vector<izenelib::util::UString>& resultTokens,
                    bool useOriginalQuery = true
                    );

            ///
            /// @brief Refer getMergedUniqueTokens(
            ///     const std::vector<izenelib::util::UString>& inputTokens,
            ///     const izenelib::util::UString& rawString,
            ///     boost::shared_ptr<LAManager> laManager,
            ///     std::vector<izenelib::util::UString>& resultTokens,
            ///     bool useOriginalQuery)
            ///
            static void getMergedUniqueTokens(
                    const izenelib::util::UString& rawString,
                    boost::shared_ptr<LAManager> laManager,
                    std::vector<izenelib::util::UString>& resultTokens,
                    bool useOriginalQuery = true
                    );

    }; // end - class QueryFunc

    class UpdatableRestrictDict : public la::UpdatableDict
    {
    public:

        UpdatableRestrictDict( unsigned int lastModifiedTime = 0 ) :
                  lastModifiedTime_( lastModifiedTime )
        {
        }

        /**
         * @bried Perform updating
         * @param path the dictionary path
         * @param lastModifiedTime last modified time
         * @return 0 indicates success and others indicates fails
         */
        virtual int update( const char* path, unsigned int lastModifiedTime )
        {
            if( lastModifiedTime_ != lastModifiedTime )
            {
                QueryUtility::reloadRestrictTermDictionary();
                lastModifiedTime_ = lastModifiedTime;
            }
            return 1;
        }

    private:

        /** current modified time */
        unsigned int lastModifiedTime_;

    };

} // end - namespace sf1r

#endif // _QMCOMMONFUNC_H_

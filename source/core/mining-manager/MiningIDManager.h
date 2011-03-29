/// @file MiningIDManager.h
/// @date 2010-06-21
/// @author Jia Guo
/// @brief This class defined the idmanager for mining manager
///
/// @details
/// - Log

#ifndef _MINING_ID_MANAGER_H_
#define _MINING_ID_MANAGER_H_

#include <ir/id_manager/IDManager.h>
#include <common/type_defs.h>
#include <string>
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <util/ustring/algo.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include "util/TermUtil.hpp"
#include <la-manager/LAManager.h>
#include <limits>

namespace sf1r
{
class MiningIDManager : public boost::noncopyable
{
    public:
        MiningIDManager(const std::string& path, boost::shared_ptr<LAManager>& laManager);
  
        ~MiningIDManager();
        
        
        uint32_t getTermIdByTermString(const izenelib::util::UString& ustr, char tag);

        bool getTermIdByTermString(const izenelib::util::UString& ustr, char tag, uint32_t& termId);

        bool getTermStringByTermId(uint32_t termId, izenelib::util::UString& ustr);

        void put(uint32_t termId, const izenelib::util::UString& ustr);

        bool isKP(uint32_t termId);
        
        void getAnalysisTermIdList(const izenelib::util::UString& ustr, std::vector<uint32_t>& termIdList);
        
        void getAnalysisTermIdList2(const izenelib::util::UString& ustr, std::vector<uint32_t>& termIdList);
        
        void getAnalysisTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList);
        
        void getFilteredAnalysisTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList);
        
        void getAnalysisTermList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& strList, std::vector<uint32_t>& termIdList);
        
        void getAnalysisTermList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList);

        void getAnalysisTermIdList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<uint32_t>& idList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList);
        
        void getAnalysisTermIdListForMatch(const izenelib::util::UString& ustr, std::vector<uint32_t>& idList);

        void flush();

        void close();
        
        bool isAutoInsert();
        
    private:
        static char getTagChar_( const std::string& posStr);
        
        
    private:        
        izenelib::ir::idmanager::HDBIDStorage< izenelib::util::UString, uint32_t>* strStorage_;
        boost::mutex mutex_;
        string path_;
        boost::shared_ptr<LAManager> laManager_;
        AnalysisInfo analysisInfo_;
        AnalysisInfo analysisInfo2_;
};
}
#endif


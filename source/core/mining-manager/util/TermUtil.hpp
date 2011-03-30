///
/// @file TermUtil.hpp
/// @brief Utility for term and id conversion
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-12-14
/// @date
///

#ifndef TERMUTIL_HPP_
#define TERMUTIL_HPP_
#include <common/type_defs.h>
// #include "TermUtilImpl.hpp"
namespace sf1r
{

class TermUtil
{

public:

    inline static uint64_t makeQueryId(const izenelib::util::UString& queryStr)
    {
        return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(queryStr);
    }

//             inline static termid_t getTermId(const izenelib::util::UString& ustring, char tag)
//             {
//                 return TermUtilImpl::getTermId(ustring, tag);
//             }
//
//             inline static termid_t getTermId(const izenelib::util::UString& ustring)
//             {
//                 return termUtilImpl_->getTermId(ustring);
//             }
//
//             inline static void getSplitTermIdList(const izenelib::util::UString& ustr, std::vector<termid_t>& termIdList)
//             {
//                 termUtilImpl_->getSplitTermIdList(ustr, termIdList);
//             }
//
//             inline static void getSplitTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList)
//             {
//                 termUtilImpl_->getSplitTermStringList(ustr, termStrList);
//             }
//
//             inline static void getAnalysisTermIdList(const izenelib::util::UString& str, std::vector<termid_t>& termIdList)
//             {
//                 termUtilImpl_->getAnalysisTermIdList(str, termIdList);
//             }
//
//             inline static void getAnalysisTermList(const izenelib::util::UString& str,
//             std::vector<izenelib::util::UString>& termList,
//             std::vector<termid_t>& termIdList)
//             {
//                 termUtilImpl_->getAnalysisTermList(str, termList, termIdList);
//             }
//
//             inline static void getAnalysisTermList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<uint32_t>& idList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList)
//             {
//                 termUtilImpl_->getAnalysisTermList(str, termList, idList, posInfoList, positionList);
//             }
//
//
//             inline static void getAnalysisTermIdListForRecommend(const izenelib::util::UString& ustr, std::vector<termid_t>& termIdList)
//             {
//                 termUtilImpl_->getAnalysisTermIdListForRecommend(ustr, termIdList);
//             }
//
//             inline static void getAnalysisTermIdListForRecommend(const izenelib::util::UString& ustr,
//             std::vector<izenelib::util::UString>& strList, std::vector<termid_t>& idList)
//             {
//                 termUtilImpl_->getAnalysisTermIdListForRecommend(ustr, strList, idList);
//             }
//
//             inline static void getAnalysisTermIdListForRecommend(const izenelib::util::UString& ustr, std::vector<termid_t>& primaryTermIdList, std::vector<termid_t>& secondaryTermIdList)
//             {
//                 termUtilImpl_->getAnalysisTermIdListForRecommend(ustr, primaryTermIdList);
//             }
//
//
//
//
//
//             inline static void setTermUtilImpl(boost::shared_ptr<TermUtilImpl>& termUtilImpl)
//             {
//                 termUtilImpl_ = termUtilImpl;
//             }
//
//
//
//         private:
//             static boost::shared_ptr<TermUtilImpl> termUtilImpl_;
};
}

#endif

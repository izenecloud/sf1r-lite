///
/// @file TermUtilImpl.hpp
/// @brief The implement of TernUtil
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-02-26
/// @date
///

#ifndef TERMUTILIMPL_HPP_
#define TERMUTILIMPL_HPP_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <mining-manager/taxonomy-generation-submanager/TgTypes.h>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include <la-manager/LAManager.h>

namespace sf1r
{

class TermUtilImpl
{
public:
    TermUtilImpl()
    {
    }
    TermUtilImpl(boost::shared_ptr<LAManager>& laManager, const AnalysisInfo& analysisInfo)
            :laManager_(laManager), analysisInfo_(analysisInfo)
    {
    }

public:

    inline static termid_t getTermId(const izenelib::util::UString& ustring, char tag)
    {
        static termid_t max = ((termid_t)(-1))>>1;
        bool canBeSingleLabel = false;
        if ( tag == 'Z' )
        {
            canBeSingleLabel = true;
        }
        termid_t flag = canBeSingleLabel? (((termid_t)(1))<<(sizeof(termid_t)*8-1)): 0;
        termid_t termId = (izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(ustring)
                           & max)
                          |flag;
        if ( termId == 0 ) ++termId;
        return termId;
    }

    inline termid_t getTermId(const izenelib::util::UString& ustring)
    {
        std::vector<termid_t> termIdList;
        getAnalysisTermIdList(ustring, termIdList);
        termid_t ret = 0;
        if (termIdList.size() > 0)
        {
            ret = termIdList[0];
        }
        else
        {
            ret = getTermId(ustring, 'F');
        }
        return ret;
    }

    inline void getSplitTermIdList(const izenelib::util::UString& ustr, std::vector<termid_t>& termIdList)
    {
        std::vector<izenelib::util::UString> ulist;
        izenelib::util::Algorithm<izenelib::util::UString>::take_between_mark(ustr, 0x20, ulist);
        BOOST_FOREACH(izenelib::util::UString& u, ulist)
        {
//                    u.toLowerString();

            if (u.length()>0)
            {
                termIdList.push_back(getTermId(u));
            }
        }
    }

    inline static void getSplitTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList)
    {
        std::vector<izenelib::util::UString> ulist;
        izenelib::util::Algorithm<izenelib::util::UString>::take_between_mark(ustr, 0x20, ulist);
        BOOST_FOREACH(izenelib::util::UString& u, ulist)
        {
//                    u.toLowerString();

            if (u.length()>0)
            {
                termStrList.push_back(u);
            }
        }
    }

    inline void getAnalysisTermIdList(const izenelib::util::UString& ustr, std::vector<termid_t>& termIdList)
    {
        la::TermList termList;
        bool b = false;
        b = laManager_->getIndexThreadSafeTermList(ustr, analysisInfo_, termList);
        if (b)
        {
            la::TermList::iterator it = termList.begin();
            while (it != termList.end())
            {
                termid_t termId = getTermId(it->text_, getTagChar(it->pos_) );
                termIdList.push_back(termId);
                it++;
            }

        }
    }

    inline void getAnalysisTermIdListForRecommend(const izenelib::util::UString& ustr, std::vector<termid_t>& primaryTermIdList)
    {
        la::TermList termList;
        bool b = false;
        b = laManager_->getIndexThreadSafeTermList(ustr, analysisInfo_, termList);
        if (b)
        {
            la::TermList::iterator it = termList.begin();
            izenelib::util::UString lastText;
            bool bLastChinese = false;
            while (it != termList.end())
            {
                char tag = getTagChar(it->pos_);
                if ( tag == 'F' )
                {
//                             it->text_.toLowerString();
                }
                termid_t termId = getTermId(it->text_, tag );
                if ( tag != 'C' )//is not chinese
                {
                    primaryTermIdList.push_back(termId);
                    bLastChinese = false;
                }
                else
                {
                    if (bLastChinese)
                    {
                        izenelib::util::UString chnBigram(lastText);
//                                 chnBigram.append(lastText);
                        chnBigram.append(it->text_);
                        termid_t bigramTermId = getTermId(chnBigram, 'C' );
                        primaryTermIdList.push_back(bigramTermId);
                    }
                    bLastChinese = true;
                    lastText = it->text_;
                }
                it++;
            }
        }
    }

    inline void getAnalysisTermIdListForRecommend(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& strList, std::vector<termid_t>& idList)
    {
        la::TermList termList;
        bool b = false;
        b = laManager_->getIndexThreadSafeTermList(ustr, analysisInfo_, termList);
        if (b)
        {
            la::TermList::iterator it = termList.begin();
            izenelib::util::UString lastText;
            bool bLastChinese = false;
            while (it != termList.end())
            {
                char tag = getTagChar(it->pos_);
                if ( tag == 'F' )
                {
//                             it->text_.toLowerString();
                }
                termid_t termId = getTermId(it->text_, tag );
                if ( tag != 'C' )//is not chinese
                {
                    strList.push_back(it->text_);
                    idList.push_back(termId);
                    bLastChinese = false;
                }
                else
                {
                    if (bLastChinese)
                    {
                        izenelib::util::UString chnBigram(lastText);
//                                 chnBigram.append(lastText);
                        chnBigram.append(it->text_);
                        strList.push_back(chnBigram);
                        termid_t bigramTermId = getTermId(chnBigram, 'C' );
                        idList.push_back(bigramTermId);
                    }
                    bLastChinese = true;
                    lastText = it->text_;
                }
                it++;
            }
        }
    }

    inline void getAnalysisTermList(const izenelib::util::UString& ustr,
                                    std::vector<izenelib::util::UString>& termList,
                                    std::vector<termid_t>& termIdList)
    {
        la::TermList terms;
        bool b = false;
        b = laManager_->getIndexThreadSafeTermList(ustr, analysisInfo_, terms);
        if (b)
        {
            la::TermList::iterator it = terms.begin();
            while (it != terms.end())
            {
                termid_t termId = getTermId(it->text_, getTagChar(it->pos_) );
                termIdList.push_back(termId);
                termList.push_back(it->text_);
                it++;
            }

        }
    }

    inline void getAnalysisTermList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<uint32_t>& idList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList)
    {
        la::TermList terms;
        bool b = false;
        b = laManager_->getIndexThreadSafeTermList(str, analysisInfo_, terms);
        if (b)
        {
            la::TermList::iterator it = terms.begin();
            while (it != terms.end())
            {
                char tag = getTagChar(it->pos_);
                termid_t termId = getTermId(it->text_, tag );
                idList.push_back(termId);
                termList.push_back(it->text_);
                posInfoList.push_back(tag);
                positionList.push_back( it->wordOffset_);
                it++;
            }

        }
    }



    inline static char getTagChar( const std::string& posStr)
    {
        if ( posStr == "NFG" || posStr == "NFU" )
        {
            return 'Z';
        }
        else if (posStr.length() > 0 )
        {
            return posStr[0];
        }
        else
        {
            return '@';
        }
    }

    inline boost::mutex& getMutex()
    {
        return mutex_;
    }


private:
    boost::shared_ptr<LAManager> laManager_;
    AnalysisInfo analysisInfo_;
    boost::mutex mutex_;
};
}

#endif

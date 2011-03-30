#include "MiningIDManager.h"
#include "taxonomy-generation-submanager/TgTypes.h"
#include "MiningException.hpp"
using namespace sf1r;

MiningIDManager::MiningIDManager(const std::string& path, boost::shared_ptr<LAManager>& laManager)
        :path_(path), laManager_(laManager)
{
    strStorage_=new izenelib::ir::idmanager::HDBIDStorage< izenelib::util::UString, uint32_t>(path+"/id_ustring_map");

    analysisInfo_.analyzerId_ = "la_mia";
    analysisInfo_.tokenizerNameList_.insert("tok_divide");
//            analysisInfo2_.analyzerId_="la_bi_mia";
    analysisInfo2_.analyzerId_="la_sia";
    analysisInfo2_.tokenizerNameList_.insert("tok_divide");

    la::LA* mia_la = laManager->get_la(analysisInfo_);
    if ( mia_la == NULL )
    {
        throw MiningConfigException("la_mia config is not correct");
    }
    la::LA* mia_la_sia = laManager->get_la(analysisInfo2_);
    if ( mia_la_sia == NULL )
    {
        throw MiningConfigException("la_sia config is not correct");
    }
}

MiningIDManager::~MiningIDManager()
{
    if (strStorage_)
        delete strStorage_;
}


uint32_t MiningIDManager::getTermIdByTermString(const izenelib::util::UString& ustr, char tag)
{
    static uint32_t max = ((termid_t)(-1))>>1;
    bool canBeSingleLabel = false;
    if ( tag == idmlib::util::IDMTermTag::KOR_COMP_NOUN )
    {
        canBeSingleLabel = true;
    }
    uint32_t flag = canBeSingleLabel? (((termid_t)(1))<<(sizeof(termid_t)*8-1)): 0;
    uint32_t termId = (izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(ustr)
                       & max)
                      |flag;
    if ( termId == 0 ) ++termId;
    if ( isAutoInsert() )
    {
        put(termId, ustr);
    }
    return termId;
}

bool MiningIDManager::getTermIdByTermString(const izenelib::util::UString& ustr, char tag, uint32_t& termId)
{
    termId = getTermIdByTermString(ustr, tag);

    return true;
}

bool MiningIDManager::getTermStringByTermId(uint32_t termId, izenelib::util::UString& ustr)
{
    boost::mutex::scoped_lock lock(mutex_);
    return strStorage_->get(termId, ustr);
}

void MiningIDManager::put(uint32_t termId, const izenelib::util::UString& ustr)
{
    boost::mutex::scoped_lock lock(mutex_);
    strStorage_->put(termId, ustr);

}

bool MiningIDManager::isKP(uint32_t termId)
{
    return ((termId & (((termid_t)(1))<<(sizeof(termid_t)*8-1)))>0);
}

void MiningIDManager::getAnalysisTermIdList(const izenelib::util::UString& ustr, std::vector<uint32_t>& termIdList)
{
    la::TermList termList;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo_, termList);
    if (b)
    {
        la::TermList::iterator it = termList.begin();
        while (it != termList.end())
        {
            termid_t termId = getTermIdByTermString(it->text_, getTagChar_(it->pos_) );
            termIdList.push_back(termId);
            it++;
        }

    }
}

void MiningIDManager::getAnalysisTermIdList2(const izenelib::util::UString& ustr, std::vector<uint32_t>& termIdList)
{
    la::TermList termList;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo2_, termList);
    if (b)
    {
        la::TermList::iterator it = termList.begin();
        while (it != termList.end())
        {
            termid_t termId = getTermIdByTermString(it->text_, getTagChar_(it->pos_) );
            termIdList.push_back(termId);
            it++;
        }

    }
}

void MiningIDManager::getFilteredAnalysisTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList)
{
    la::TermList terms;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo_, terms);
    if (b)
    {
        la::TermList::iterator it = terms.begin();
        while (it != terms.end())
        {
            bool valid = true;
            if ( it->text_.length()==1 )
            {
                if ( !it->text_.isKoreanChar(0) && !it->text_.isChineseChar(0) )
                {
                    valid = false;
                }
            }
            if ( valid) termStrList.push_back(it->text_);
            it++;
        }

    }

}

void MiningIDManager::getAnalysisTermStringList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& termStrList)
{
    la::TermList terms;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo_, terms);
    if (b)
    {
        la::TermList::iterator it = terms.begin();
        while (it != terms.end())
        {
            termStrList.push_back(it->text_);
            it++;
        }

    }
}

void MiningIDManager::getAnalysisTermList(const izenelib::util::UString& ustr, std::vector<izenelib::util::UString>& strList, std::vector<uint32_t>& termIdList)
{
    la::TermList terms;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo_, terms);
    if (b)
    {
        la::TermList::iterator it = terms.begin();
        while (it != terms.end())
        {
            termid_t termId = getTermIdByTermString(it->text_, getTagChar_(it->pos_) );
            termIdList.push_back(termId);
            strList.push_back(it->text_);
            it++;
        }

    }
}

void MiningIDManager::getAnalysisTermList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList)
{
    la::TermList terms;
    bool b = false;
    b = laManager_->getTermList(str, analysisInfo_, terms);

    if (b)
    {
//         std::string allStr;
//         str.convertString(allStr, izenelib::util::UString::UTF_8);
//         std::cout<<"{{ALLSTR}} "<<allStr<<std::endl;
//         std::cout<<"{{ALLEND}}"<<std::endl;
        la::TermList::iterator it = terms.begin();
        while (it != terms.end())
        {
//             std::string _str;
//             it->text_.convertString(_str, izenelib::util::UString::UTF_8);
//             std::cout<<"["<<it->wordOffset_<<","<<_str<<","<<it->pos_<<"]";

            char tag = getTagChar_(it->pos_);
            termList.push_back(it->text_);
            posInfoList.push_back(tag);
            positionList.push_back( it->wordOffset_);
            it++;
        }
//         std::cout<<std::endl;


    }
}

void MiningIDManager::getAnalysisTermIdList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& termList, std::vector<uint32_t>& idList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList)
{
    la::TermList terms;
    bool b = false;
    b = laManager_->getTermList(str, analysisInfo_, terms);
    if (b)
    {
        la::TermList::iterator it = terms.begin();
        while (it != terms.end())
        {
            char tag = getTagChar_(it->pos_);
            termid_t termId = getTermIdByTermString(it->text_, tag );
            idList.push_back(termId);
            termList.push_back(it->text_);
            posInfoList.push_back(tag);
            positionList.push_back( it->wordOffset_);
            it++;
        }

    }
}

void MiningIDManager::getAnalysisTermIdListForMatch(const izenelib::util::UString& ustr, std::vector<uint32_t>& idList)
{
    la::TermList termList;
    bool b = false;
    b = laManager_->getTermList(ustr, analysisInfo_, termList);
    if (b)
    {
        la::TermList::iterator it = termList.begin();
        izenelib::util::UString lastText;
        bool bLastChinese = false;
        while (it != termList.end())
        {
            char tag = getTagChar_(it->pos_);
            if ( tag == idmlib::util::IDMTermTag::ENG )
            {
//                             it->text_.toLowerString();
            }
            termid_t termId = getTermIdByTermString(it->text_, tag );
            if ( tag != idmlib::util::IDMTermTag::CHN )//is not chinese
            {
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
                    termid_t bigramTermId = getTermIdByTermString(chnBigram, idmlib::util::IDMTermTag::CHN );
                    idList.push_back(bigramTermId);
                }
                bLastChinese = true;
                lastText = it->text_;
            }
            it++;
        }
    }
}

void MiningIDManager::flush()
{
    strStorage_->flush();
}

void MiningIDManager::close()
{
    strStorage_->close();
}

bool MiningIDManager::isAutoInsert()
{
    return false;
}

char MiningIDManager::getTagChar_( const std::string& posStr)
{
    if ( posStr == "NFG" || posStr == "NFU" )
    {
        return idmlib::util::IDMTermTag::KOR_LOAN;
    }
    else if (posStr.length() > 0 )
    {
        return posStr[0];
    }
    else
    {
        return idmlib::util::IDMTermTag::OTHER;
    }
}

/**@file   QueryCorrectionSubmanager.cpp
 * @brief  source file of Query Correction
 * @author Jinglei Zhao&Jinli Liu
 * @date   2009-08-21
 * @details
 * - Log
 *     - 2009.09.26 add new candidate generation
 *     - 2009.10.08 add new error model for english
 *     - 2009.11.27 add isEnglishWord() and isKoreanWord() to check if one word is mixure
 *      -2010.03.05 add log manager periodical worker.
 */
#include "Util.h"
#include "QueryCorrectionSubmanager.h"
#include <common/SFLogger.h>
#include <util/ustring/ustr_tool.h>
#include <boost/algorithm/string/trim.hpp>

namespace
{

static const float SMOOTH = 0.000001;
static const float FACTOR = 0.05;
static const int UNIGRAM_FREQ_THRESHOLD = 3;
static const int BIGRAM_FREQ_THRESHOLD = 3;
static const int UNIGRAM_FACTOR = 10;
static const int BIGRAM_FACTOR = 1;
static const unsigned int CHINESE_CORRECTION_LENGTH = 8;

}
using namespace izenelib::util::ustring_tool;
namespace sf1r
{

_QueryCorrectionSubmanagerParam QueryCorrectionSubmanagerParam::param_;

QueryCorrectionSubmanager::QueryCorrectionSubmanager
(const string& path, const std::string& workingPath, bool enableEK, bool enableChn, int ed)
:path_(path), workingPath_(workingPath)
, enableEK_(enableEK), enableChn_(enableChn)
, activate_(false)
, cmgr_(path_+"/cn", workingPath_), ekmgr_(path, workingPath, ed)
, has_new_inject_(false)
{
    initialize();
}

QueryCorrectionSubmanager& QueryCorrectionSubmanager::getInstance()
{
    _QueryCorrectionSubmanagerParam& param =
        QueryCorrectionSubmanagerParam::get();
    static QueryCorrectionSubmanager qcManager(param.path_, param.workingPath_,
            param.enableEK_, param.enableChn_);
    return qcManager;
}

// bool QueryCorrectionSubmanager::stopHere(
//     const izenelib::util::UString& queryUString)
// {
//     bool is = true;
//     std::vector<UString> queryTokens;
// 
//     string queryStr;
//     queryUString.convertString(queryStr, UString::UTF_8);
//     if (cmgr_.inPinyinDict(queryStr))
//         return false;
// 
//     getTokensFromUString(UString::UTF_8,' ', queryUString, queryTokens);
// //	Tokenize(queryUString, queryTokens);
// 
//     uint32_t chineseCount = 0;
//     uint32_t ekCount = 0;
//     for (size_t i = 0; i < queryUString.length(); i++)
//     {
//         if (queryUString.isChineseChar(i))
//         {
//             ++chineseCount;
//         }
//         else
//         {
//             ++ekCount;
//         }
//     }
//     // if( !enableChn_ && chineseCount>0 ) return true;
//     // if( !enableEK_ && ekCount>0 ) return true;
//     if (chineseCount > CHINESE_CORRECTION_LENGTH)
//         return true;
// 
//     vector<UString>::iterator it_token = queryTokens.begin();
//     for (; it_token != queryTokens.end(); it_token++)
//     {
//         string str;
//         it_token->convertString(str, izenelib::util::UString::UTF_8);
// 
//         //if it is one word in Chinese dictionary, Chinese
//         //QueryCorrection is firstly processed.
//         (*it_token).toLowerString();
//         if (queryTokens.size() == 1 && cmgr_.inPinyinDict(str))
//         {
//             return false;
//         }
//         bool b1 = ekmgr_.inDict(*it_token);
//         bool b2 = cmgr_.inChineseDict(str);
//         if ( b2 )
//         {
//             return false;
//         }
//         else
//         {
//             if ( !b1 )
//                 is = false;
//         }
// 
//     }
//     return is;
// }


//Initialize some member variables
bool QueryCorrectionSubmanager::initialize()
{
    std::cout<< "Start Speller construction!" << std::endl;
    activate_ = true;
    if (!boost::filesystem::exists(path_) || !boost::filesystem::is_directory(
                path_))
    {
        std::string
        msg =
            "Initialize query correction failed, please ensure that you set the correct path in configuration file.";
        sflog->error(SFL_MINE, msg.c_str());
        activate_ = false;
        return false;
    }

    if (enableEK_)
    {
        ekmgr_.initialize();
    }

    if (enableChn_)
    {
        if(!cmgr_.Load())
        {
            std::cerr<<"Load failed for Chinese query correction"<<std::endl;
            activate_ = false;
            return false;
        }
    }

    {
        boost::mutex::scoped_lock scopedLock(logMutex_);
        if (enableEK_)
        {
            ekmgr_.warmUp();
        }

    }
    
    //load inject
    std::string inject_file = workingPath_+"/inject_data.txt";
    std::vector<izenelib::util::UString> str_list;
    std::ifstream ifs(inject_file.c_str());
    std::string line;
    while( getline(ifs, line) )
    {
        boost::algorithm::trim(line);
        if(line.length()==0)
        {
            //do with str_list;
            if(str_list.size()>=1)
            {
                std::string str_query;
                str_list[0].convertString(str_query, izenelib::util::UString::UTF_8);
                izenelib::util::UString result;
                if(str_list.size()>=2)
                {
                    result = str_list[1];
                }
                inject_data_.insert(std::make_pair(str_query, result));
            }
            str_list.resize(0);
            continue;
        }
        str_list.push_back( izenelib::util::UString(line, izenelib::util::UString::UTF_8));
    }
    ifs.close();

    std::cout << "End Speller construction!" << std::endl;

    return true;
}

QueryCorrectionSubmanager::~QueryCorrectionSubmanager()
{
    DLOG(INFO) << "... Query Correction module is destroyed" << std::endl;
}

bool QueryCorrectionSubmanager::getRefinedToken_(const std::string& collectionName, const izenelib::util::UString& token, izenelib::util::UString& result)
{
    if (enableChn_)
    {
        std::vector<izenelib::util::UString> vec_result;
        if ( cmgr_.GetResult(token, vec_result) )
        {
            if(vec_result.size()>0)
            {
                result = vec_result[0];
                return true;
            }
            return false;
        }
    }
    if (enableEK_)
    {
        if ( ekmgr_.getRefinedQuery(collectionName, token, result) )
        {
            if(result.length()>0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    return false;
}


//The public interface, when user input wrong query, given the correct refined query.
bool QueryCorrectionSubmanager::getRefinedQuery(const UString& queryUString,
        UString& refinedQueryUString)
{
    return getRefinedQuery("", queryUString, refinedQueryUString);
}

bool QueryCorrectionSubmanager::getRefinedQuery(
    const std::string& collectionName, const UString& queryUString,
    UString& refinedQueryUString)
{
//     std::vector<izenelib::util::UString> vec_result;
//     if ( cmgr_.GetResult(queryUString, vec_result) )
//     {
//         if(vec_result.size()>0)
//         {
//             refinedQueryUString = vec_result[0];
//             return true;
//         }
//         
//     }
//     return false;

    std::string str_query;
    queryUString.convertString(str_query, izenelib::util::UString::UTF_8);
    boost::algorithm::to_lower(str_query);
    boost::unordered_map<std::string, izenelib::util::UString>::iterator it = inject_data_.find(str_query);
    if(it!=inject_data_.end())
    {
        refinedQueryUString = it->second;
        return true;
    }
    if (queryUString.empty() || !activate_)
    {
        return false;
    }
    if (!enableEK_ && !enableChn_)
    {
        return false;
    }

    CREATE_SCOPED_PROFILER(getRealRefinedQuery, "QueryCorrectionSubmanager",
                           "QueryCorrectionSubmanager :: getRealRefinedQuery");

    // std::string sourceString( env.queryString_ );

    typedef tokenizer<char_separator<char> > tokenizers;
    char_separator<char> sep(QueryManager::seperatorString.c_str());//In order to omit ' punction.As we need to support words with suffix 's

    std::string queryStr;
    queryUString.convertString(queryStr, izenelib::util::UString::UTF_8);
    tokenizers stringTokenizer(queryStr, sep);

    // Tokenizing and apply query correction.
//     izenelib::util::UString originalToken;
    bool bRefined = false;
    bool first = true;
    for (tokenizers::iterator iter = stringTokenizer.begin(); iter
            != stringTokenizer.end(); iter++)
    {
        if (!first)
        {
            refinedQueryUString += ' ';
        }
        izenelib::util::UString token(*iter, izenelib::util::UString::UTF_8);
        izenelib::util::UString refined_token;
        if(getRefinedToken_(collectionName, token, refined_token) )
        {
            refinedQueryUString += refined_token;
            bRefined = true;
        }
        else
        {
            refinedQueryUString += token;
        }
        
        first = false;
    }
    
    if(bRefined)
    {
        return true;
    }
    else
    {
        refinedQueryUString.clear();
        return false;
    }
    
//     if (stopHere(queryUString) )
//     {
//         return false;
//     }
}

bool QueryCorrectionSubmanager::getPinyin(
    const izenelib::util::UString& hanzis, std::vector<
    izenelib::util::UString>& pinyin)
{
//     return cmgr_.getPinyin(hanzis, pinyin);
    return false;
}
// 
// bool QueryCorrectionSubmanager::pinyinSegment(const string& str,
//         std::vector<string>& result)
// {
// 
//     std::vector<std::vector<string> > vpy;
//     if ( !cmgr_.pinyinSegment(str, vpy) )
//     {
//         result.push_back(str);
//         return true;
//     }
// 
//     size_t minSize = 1000;
//     for (size_t i=0; i<vpy.size(); i++)
//     {
//         if (vpy[i].size() < minSize)
//         {
//             minSize = vpy[i].size() ;
//             result = vpy[i];
//         }
//         for (size_t j=0; j<vpy[i].size(); j++)
//             cout<<vpy[i][j]<<" -> ";
//         cout<<endl;
//     }
//     return true;
// }
// 
// 
bool QueryCorrectionSubmanager::isPinyin(const izenelib::util::UString& str)
{
//     std::string tempString;
//     str.convertString(tempString, izenelib::util::UString::UTF_8);
//     return cmgr_.isPinyin(tempString);
    return false;
}
// 
void QueryCorrectionSubmanager::updateCogramAndDict(const std::list<std::pair<izenelib::util::UString, uint32_t> >& recentQueryList)
{
    updateCogramAndDict("", recentQueryList);
}

void QueryCorrectionSubmanager::updateCogramAndDict(const std::string& collectionName, const std::list<std::pair<izenelib::util::UString, uint32_t> >& recentQueryList)
{
    boost::mutex::scoped_lock scopedLock(logMutex_);

    DLOG(INFO)<<"updateCogramAndDict..."<<endl;
    const std::list < std :: pair < izenelib::util::UString, uint32_t> >& queryList = recentQueryList;
    std::list < std :: pair < izenelib::util::UString, uint32_t> >::const_iterator lit;
    //no collection independent
    cmgr_.Update(queryList);

}

void QueryCorrectionSubmanager::Inject(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    std::string str_query;
    query.convertString(str_query, izenelib::util::UString::UTF_8);
    boost::algorithm::trim(str_query);
    if(str_query.empty()) return;
    boost::algorithm::to_lower(str_query);
    std::cout<<"Inject query correction : "<<str_query<<std::endl;
    inject_data_.erase( str_query );
    inject_data_.insert(std::make_pair( str_query, result) );
    has_new_inject_ = true;
}

void QueryCorrectionSubmanager::FinishInject()
{
    if(!has_new_inject_) return;
    std::string inject_file = workingPath_+"/inject_data.txt";
    if(boost::filesystem::exists( inject_file) )
    {
        boost::filesystem::remove_all( inject_file);
    }
    std::ofstream ofs(inject_file.c_str());
    boost::unordered_map<std::string, izenelib::util::UString>::iterator it = inject_data_.begin();
    while(it!= inject_data_.end())
    {
        std::string result;
        it->second.convertString(result, izenelib::util::UString::UTF_8);
        ofs<<it->first<<std::endl;
        ofs<<result<<std::endl;
        ofs<<std::endl;
        ++it;
    }
    ofs.close();
    has_new_inject_ = false;
    std::cout<<"Finish inject query correction."<<std::endl;
}

}/*namespace sf1r*/


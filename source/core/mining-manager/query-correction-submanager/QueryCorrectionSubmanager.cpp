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
#include "QueryCorrectionSubmanager.h"
#include <boost/algorithm/string.hpp>

using namespace izenelib::util::ustring_tool;
namespace sf1r
{

std::string QueryCorrectionSubmanager::system_resource_path_;
std::string QueryCorrectionSubmanager::system_working_path_;

boost::shared_ptr<EkQueryCorrection> QueryCorrectionSubmanager::ekmgr_;

boost::unordered_map<std::string, izenelib::util::UString> QueryCorrectionSubmanager::global_inject_data_;

QueryCorrectionSubmanager::QueryCorrectionSubmanager(const string& queryDataPath, bool enableEK, bool enableChn, int ed)
    : queryDataPath_(queryDataPath)
    , enableEK_(enableEK), enableChn_(enableChn)
    , ed_(ed)
    , activate_(false)
    , default_inject_data_(queryDataPath_.empty() ? global_inject_data_ : collection_inject_data_)
    , has_new_inject_(false)
{
    static bool FirstRun = true;
    if (FirstRun)
    {
        FirstRun = false;

        idmlib::qc::CnQueryCorrection::res_dir_ = system_resource_path_ + "/speller-support/cn";
        EkQueryCorrection::path_ = system_resource_path_ + "/speller-support";
        EkQueryCorrection::dictEN_ = system_resource_path_ + "/speller-support/dictionary_english";
        EkQueryCorrection::dictKR_ = system_resource_path_ + "/speller-support/dictionary_korean";
    }

    initialize();
}

QueryCorrectionSubmanager& QueryCorrectionSubmanager::getInstance()
{
    static QueryCorrectionSubmanager qcManager("", true, true);
    return qcManager;
}

//Initialize some member variables
bool QueryCorrectionSubmanager::initialize()
{
    std::cout << "Start Speller construction!" << std::endl;
    activate_ = true;

    if (enableChn_)
    {
        if (queryDataPath_.empty())
            cmgr_.reset(new idmlib::qc::CnQueryCorrection());
        else
            cmgr_.reset(new idmlib::qc::CnQueryCorrection(queryDataPath_ + "/query-correction"));
        if (!cmgr_->Load())
        {
            std::cerr << "Failed loading resources for Chinese query correction" << std::endl;
            activate_ = false;
            return false;
        }
    }

    {
        boost::mutex::scoped_lock scopedLock(logMutex_);
        if (enableEK_ && !ekmgr_)
        {
            ekmgr_.reset(new EkQueryCorrection(ed_));
            if (!ekmgr_->initialize())
            {
                std::cerr << "Failed loading resources for English and Korean query correction" << std::endl;
                activate_ = false;
                return false;
            }
            ekmgr_->warmUp();
        }
    }

    //load global inject
    {
        boost::mutex::scoped_lock scopedLock(logMutex_);
        if (global_inject_data_.empty())
        {
            std::string inject_file = system_working_path_ + "/query-correction/correction_inject.txt";
            std::vector<izenelib::util::UString> str_list;
            std::ifstream ifs(inject_file.c_str());
            std::string line;
            while (getline(ifs, line))
            {
                boost::algorithm::trim(line);
                if (line.length() == 0)
                {
                    //do with str_list;
                    if (str_list.size() >= 1)
                    {
                        std::string str_query;
                        str_list[0].convertString(str_query, izenelib::util::UString::UTF_8);
                        izenelib::util::UString result;
                        if (str_list.size() >= 2)
                        {
                            result = str_list[1];
                        }
                        global_inject_data_.insert(std::make_pair(str_query, result));
                    }
                    str_list.resize(0);
                    continue;
                }
                str_list.push_back(izenelib::util::UString(line, izenelib::util::UString::UTF_8));
            }
            ifs.close();
        }
    }

    //load collection-specific inject
    if (!queryDataPath_.empty())
    {
        std::string inject_file = queryDataPath_ + "/query-corrction/correction_inject.txt";
        std::vector<izenelib::util::UString> str_list;
        std::ifstream ifs(inject_file.c_str());
        std::string line;
        while (getline(ifs, line))
        {
            boost::algorithm::trim(line);
            if (line.length() == 0)
            {
                //do with str_list;
                if (str_list.size() >= 1)
                {
                    std::string str_query;
                    str_list[0].convertString(str_query, izenelib::util::UString::UTF_8);
                    izenelib::util::UString result;
                    if (str_list.size() >= 2)
                    {
                        result = str_list[1];
                    }
                    collection_inject_data_.insert(std::make_pair(str_query, result));
                }
                str_list.resize(0);
                continue;
            }
            str_list.push_back(izenelib::util::UString(line, izenelib::util::UString::UTF_8));
        }
        ifs.close();
    }

    std::cout << "End Speller construction!" << std::endl;

    return true;
}

QueryCorrectionSubmanager::~QueryCorrectionSubmanager()
{
    DLOG(INFO) << "... Query Correction module is destroyed" << std::endl;
}

bool QueryCorrectionSubmanager::getRefinedToken_(const izenelib::util::UString& token, izenelib::util::UString& result)
{
    if (enableChn_)
    {
        std::vector<izenelib::util::UString> vec_result;
        if (cmgr_->GetResult(token, vec_result))
        {
            if (vec_result.size() > 0)
            {
                result = vec_result[0];
                return true;
            }
        }
    }
    if (enableEK_)
    {
        if (ekmgr_->getRefinedQuery(token, result))
        {
            if (result.length() > 0)
            {
                return true;
            }
        }
    }
    return false;
}

//The public interface, when user input wrong query, given the correct refined query.
bool QueryCorrectionSubmanager::getRefinedQuery(const UString& queryUString, UString& refinedQueryUString)
{
    if (queryUString.empty() || !activate_)
    {
        return false;
    }
    if (!enableEK_ && !enableChn_)
    {
        return false;
    }

    std::string str_query;
    queryUString.convertString(str_query, izenelib::util::UString::UTF_8);
    boost::algorithm::to_lower(str_query);
    boost::unordered_map<std::string, izenelib::util::UString>::const_iterator it = collection_inject_data_.find(str_query);
    if (it != collection_inject_data_.end())
    {
        refinedQueryUString = it->second;
        return true;
    }
    it = global_inject_data_.find(str_query);
    if (it != global_inject_data_.end())
    {
        refinedQueryUString = it->second;
        return true;
    }

    CREATE_SCOPED_PROFILER(getRealRefinedQuery, "QueryCorrectionSubmanager",
                           "QueryCorrectionSubmanager :: getRealRefinedQuery");

    typedef tokenizer<char_separator<char> > tokenizers;
    char_separator<char> sep;

    std::string queryStr;
    queryUString.convertString(queryStr, izenelib::util::UString::UTF_8);
    tokenizers stringTokenizer(queryStr, sep);

    // Tokenizing and apply query correction.
    bool bRefined = false;
    bool first = true;
    for (tokenizers::const_iterator iter = stringTokenizer.begin();
            iter != stringTokenizer.end(); ++iter)
    {
        if (!first)
        {
            refinedQueryUString.push_back(' ');
        }
        izenelib::util::UString token(*iter, izenelib::util::UString::UTF_8);
        izenelib::util::UString refined_token;
        if (getRefinedToken_(token, refined_token) )
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

    if (bRefined)
    {
        return true;
    }
    else
    {
        refinedQueryUString.clear();
        return false;
    }
}

bool QueryCorrectionSubmanager::getPinyin(
        const izenelib::util::UString& hanzi,
        std::vector<izenelib::util::UString>& pinyin)
{
    std::vector<std::string> result_list;
    cmgr_->GetPinyin(hanzi, result_list);
    for (uint32_t i=0;i<result_list.size();i++)
    {
        boost::algorithm::replace_all(result_list[i], ",", "");
        pinyin.push_back(izenelib::util::UString(result_list[i], izenelib::util::UString::UTF_8));
    }

    return pinyin.size() > 0;
}

void QueryCorrectionSubmanager::updateCogramAndDict(const std::list<QueryLogType>& queryList, const std::list<PropertyLabelType>& labelList)
{
    DLOG(INFO) << "updateCogramAndDict..." << endl;
    cmgr_->Update(queryList, labelList);
}

void QueryCorrectionSubmanager::Inject(const izenelib::util::UString& query, const izenelib::util::UString& result)
{
    std::string str_query;
    query.convertString(str_query, izenelib::util::UString::UTF_8);
    std::string str_result;
    result.convertString(str_result, izenelib::util::UString::UTF_8);
    boost::algorithm::trim(str_query);
    if(str_query.empty()) return;
    boost::algorithm::to_lower(str_query);

    if (str_result == "__delete__")
    {
    std::cout << "Delete injected keyword for query correction: " << str_query << std::endl;
        default_inject_data_.erase(str_query);
    }
    else
    {
        std::cout << "Inject keyword for query correction: " << str_query << std::endl;
        default_inject_data_[str_query] = result;
    }
    has_new_inject_ = true;
}

void QueryCorrectionSubmanager::FinishInject()
{
    if (!has_new_inject_)
        return;

    std::string dir = (queryDataPath_.empty() ? system_working_path_ : queryDataPath_) + "/query-correction";
    std::string inject_file = dir + "/correction_inject.txt";

    if (boost::filesystem::exists(dir))
    {
        if (!boost::filesystem::is_directory(dir))
        {
            boost::filesystem::remove_all(dir);
            boost::filesystem::create_directories(dir);
        }

        if (boost::filesystem::exists(inject_file))
            boost::filesystem::remove_all(inject_file);
    }
    else
        boost::filesystem::create_directories(dir);

    std::ofstream ofs(inject_file.c_str());
    for (boost::unordered_map<std::string, izenelib::util::UString>::const_iterator it = default_inject_data_.begin();
            it!= default_inject_data_.end(); ++it)
    {
        std::string result;
        it->second.convertString(result, izenelib::util::UString::UTF_8);
        ofs << it->first << std::endl;
        ofs << result << std::endl;
        ofs << std::endl;
    }
    ofs.close();

    has_new_inject_ = false;
    std::cout << "Finish inject query correction." << std::endl;
}

}/*namespace sf1r*/

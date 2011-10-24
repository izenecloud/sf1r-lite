/**
 * @file AutoFillSubManager.cpp
 *
 * @author
 * @brief The implementation file of AutoFillSubmanager
 * @details
 */
#include "AutoFillSubManager.h"
#include <mining-manager/util/LabelDistance.h>
#include <util/ustring/algo.hpp>
#include <util/ustring/ustr_tool.h>
#include <common/SFLogger.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
#include <util/filesystem.h>
#include <util/functional.h>
#include <signal.h>

namespace sf1r
{
const int MAX_READ_BUF_SIZE = 255;


AutoFillSubManager::AutoFillSubManager():dir_switcher_(NULL)
{

}

AutoFillSubManager::~AutoFillSubManager()
{
    if (autoFillTrie_ != NULL)
    {
        autoFillTrie_->flush();
        delete autoFillTrie_;
    }
    if ( dir_switcher_!= NULL)
    {
        delete dir_switcher_;
    }
}

bool AutoFillSubManager::Init(
    const std::string& fillSupportPath,
    uint32_t top,
    uint32_t queryUpdateTime)
{
    fillSupportPath_ = fillSupportPath;
    top_=top;
    queryUpdateTime_=queryUpdateTime;

    try
    {
        if (!boost::filesystem::is_directory(fillSupportPath_))
        {
            boost::filesystem::create_directories(fillSupportPath_);
        }
    }
    catch (boost::filesystem::filesystem_error& e)
    {
        sflog->error(SFL_SRCH, 110602, fillSupportPath_.c_str());
        return false;
    } // end - try-catch
    dir_switcher_ = new idmlib::util::DirectorySwitcher(fillSupportPath_);
    if ( !dir_switcher_->Open() )
    {
        std::cerr<<"Open dir switcher failed"<<std::endl;
        return false;
    }
    if (!dir_switcher_->GetCurrent(triePath_) )
    {
        std::cerr<<"Get current working dir failed"<<std::endl;
        return false;
    }

    try
    {
        autoFillTrie_ = new Trie_(triePath_, top_);
    }
    catch (const std::exception& e)
    {
        std::string errorMsg("Fail to load Autofill data file. Removing "
                             + triePath_);
        sflog->error(SFL_INIT, 110603, triePath_.c_str());
        boost::filesystem::remove_all(triePath_);
        autoFillTrie_ = new Trie_(triePath_, top_);
    }

    return true;
}

bool AutoFillSubManager::buildIndex(const std::list<ItemValueType>& queryList)
{
    if (queryList.empty())
    {
        std::cout << "no query log data to build for autofill" << std::endl;
        return true;
    }
    std::string tempTriePath;
    if (!dir_switcher_->GetNextWithDelete(tempTriePath) )
    {
        return false;
    }

    Trie_* tempTrie = NULL;
    try
    {
        if (!boost::filesystem::is_directory(tempTriePath))
        {
            boost::filesystem::create_directories(tempTriePath);
        }
        tempTrie = new Trie_(tempTriePath , top_);

        std::list<ItemValueType>::const_iterator it;
        for (it = queryList.begin(); it != queryList.end(); it++)
            buildTrieItem(tempTrie, *it);

        tempTrie->flush();
    }
    catch (...)
    {
        sflog->crit(SFL_MINE, "Building collection trie in autofill fails");
        try
        {
            if ( tempTrie )
            {
                delete tempTrie;
            }
            boost::filesystem::remove_all(tempTriePath);
        }
        catch (...) {}
        return false;
    } // end - try-catch
    {
        boost::lock_guard<boost::shared_mutex> lock(mutex_);
        Trie_* tmp = autoFillTrie_;
        autoFillTrie_ = tempTrie;
        tempTrie = tmp;
    }
    try
    {
        dir_switcher_->SetNextValid();
        delete tempTrie;
    }
    catch (const std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
        sflog->error(SFL_MINE, e.what());
        return false;
    }
    return true;
}

bool AutoFillSubManager::getAutoFillList(const izenelib::util::UString& query,
        std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    if (query.length() == 0)
        return false;
    if (autoFillTrie_ == NULL)
        return false;
    izenelib::util::UString tempQuery;
    std::vector<ItemValueType> result;
    bool ret;
    if (query.isChineseChar(0))
    {
        tempQuery = izenelib::util::Algorithm<izenelib::util::UString>::trim(query);
        boost::shared_lock<boost::shared_mutex> mLock(mutex_);
        ret = autoFillTrie_->get_item(tempQuery, list);
    }
    else if (izenelib::util::ustring_tool::processKoreanDecomposerWithCharacterCheck<
             izenelib::util::UString>(query, tempQuery))
    {
        boost::shared_lock<boost::shared_mutex> mLock(mutex_);
        ret = autoFillTrie_->get_item(tempQuery, list);
    }
    else
    {
        boost::shared_lock<boost::shared_mutex> mLock(mutex_);
        ret = autoFillTrie_->get_item(query, list);
    }

    std::cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@@list autofill result: "<<list.size()<<std::endl;

    return ret;
}

void AutoFillSubManager::updateTrie(Trie_* trie, const izenelib::util::UString& key,
                                    const ItemValueType& value)
{
    trie->add_item(key, value.get<2>(), value.get<1>(), value.get<0>());
}

void AutoFillSubManager::buildTrieItem(Trie_* trie, const ItemValueType& item)
{
    izenelib::util::UString key = item.get<2>();
    string keystr;
    key.convertString(keystr, UString::UTF_8);

    if (key.length() == 0)
        return;
    izenelib::util::UString decomposedLabel;
    if (key.isChineseChar(0))
    {
        izenelib::util::UString tempLabel;
        tempLabel = izenelib::util::Algorithm<izenelib::util::UString>::trim(key);
        updateTrie(trie, tempLabel, item);
        std::vector<izenelib::util::UString> pinyins;
        QueryCorrectionSubmanager::getInstance().getPinyin(tempLabel, pinyins);

        for (size_t i=1; i<key.length(); i++)
        {
            UString suffix = key.substr(i);
            string suffixstr;
            suffix.convertString(suffixstr, UString::UTF_8);
            updateTrie(trie, suffix, item);
        }
        for (unsigned int i = 0; i < pinyins.size(); i++)
        {
            updateTrie(trie, pinyins[i], item);
        }
    }
    else if (izenelib::util::ustring_tool::processKoreanDecomposerWithCharacterCheck<
             izenelib::util::UString>(key, decomposedLabel))
    {
        updateTrie(trie, decomposedLabel, item);
    }
    else
    {
        izenelib::util::UString tempKey(key);
        tempKey.toLowerString();
        updateTrie(trie, tempKey, item);
    }
}

void AutoFillSubManager::buildTrieItem(Trie_* trie,
                                       const izenelib::util::UString& key, const ItemValueType& item)
{
    string keystr;
    key.convertString(keystr, UString::UTF_8);

    if (key.length() == 0)
        return;
    izenelib::util::UString decomposedLabel;
    if (key.isChineseChar(0))
    {
        izenelib::util::UString tempLabel;
        tempLabel = izenelib::util::Algorithm<izenelib::util::UString>::trim(key);
        updateTrie(trie, tempLabel, item);
        std::vector<izenelib::util::UString> pinyins;
        QueryCorrectionSubmanager::getInstance().getPinyin(tempLabel, pinyins);

        for (size_t i=1; i<key.length(); i++)
        {
            UString suffix = key.substr(i);
            string suffixstr;
            suffix.convertString(suffixstr, UString::UTF_8);
            updateTrie(trie, suffix, item);
        }
        for (unsigned int i = 0; i < pinyins.size(); i++)
        {
            updateTrie(trie, pinyins[i], item);
        }
    }
    else if (izenelib::util::ustring_tool::processKoreanDecomposerWithCharacterCheck<
             izenelib::util::UString>(key, decomposedLabel))
    {
        updateTrie(trie, decomposedLabel, item);
    }
    else
    {
        izenelib::util::UString tempKey(key);
        tempKey.toLowerString();
        updateTrie(trie, tempKey, item);
    }
}

}

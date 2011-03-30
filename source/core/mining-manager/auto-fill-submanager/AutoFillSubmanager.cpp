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

// buildFromLabelFile( "/home/wps/codebase/sf1-revolution/bin/1.label" );
// buildFromLogFile( "/home/wps/codebase/sf1-revolution/bin/20101222.log" );
    return true;

//     boost::thread updateLogThread(boost::bind(
//             &AutoFillSubManager::updateLogWorker, this));
//     updateLogThread.detach();
//
//     boost::thread buildThread(boost::bind(&AutoFillSubManager::buildProcess,
//             this));
//     buildThread.detach();

}

// void AutoFillSubManager::updateLogWorker()
// {
//     while (true)
//     {
//         boost::system_time wakeupTime = boost::get_system_time()
//                 + boost::posix_time::milliseconds(queryUpdateTime_*3600*1000);
//         boost::thread::sleep(wakeupTime);
//         updateRecentLog();
//     }
// }
//

bool AutoFillSubManager::buildIndex(const std::list<std::pair<izenelib::util::UString,
                                    uint32_t> >& queryList)
{
    std::vector<boost::shared_ptr<LabelManager> > tmp;
    return buildIndex(queryList, tmp);
}

bool AutoFillSubManager::buildIndex(const std::list<std::pair<izenelib::util::UString,
                                    uint32_t> >& queryList, const std::vector<boost::shared_ptr<LabelManager> >& label_manager_list)
{
    if (queryList.size()==0 && label_manager_list.size()==0 )
    {
        std::cout<<"no data to build autofill"<<std::endl;
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

        std::list<std::pair<izenelib::util::UString, uint32_t> >::const_iterator it;
        for (it = queryList.begin(); it != queryList.end(); it++)
            buildTrieItem(tempTrie, it->first, it->second);
        //for labels
        izenelib::util::UString label_str;
        uint32_t label_df = 0;
        for (uint32_t i=0;i<label_manager_list.size();i++)
        {
            boost::shared_ptr<LabelManager> label_manager = label_manager_list[i];
            uint32_t max_label_id = label_manager->getMaxLabelID();
            for (uint32_t id=1; id<max_label_id;id++)
            {
                if ( !label_manager->getLabelStringByLabelId(id, label_str) ) continue;
                if ( !label_manager->getLabelDF(id, label_df) ) continue;
                buildTrieItem(tempTrie, label_str, label_df);
            }
        }
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
        std::vector<izenelib::util::UString>& list)
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
    trie->add_item(key, value.second, value.first);
}

void AutoFillSubManager::buildTrieItem(Trie_* trie,
                                       const izenelib::util::UString& key, uint32_t value)
{
    if (!QueryCorrectionSubmanager::getInstance().isPinyin(key))
    {
        ItemValueType item(value, key);
        buildTrieItem(trie, key, item);
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

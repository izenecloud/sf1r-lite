#include "TrieClassifier.h"
#include <boost/filesystem.hpp>

#include <fstream>
#include <queue>
#include <iostream>

namespace sf1r
{
static const std::string FORMAT_LSB = "[";
static const std::string FORMAT_RSB = "]";
static const std::string FORMAT_OR  = "/";
static const std::string FORMAT_SYNONYM = ":";
static const std::string SYNONYM_DELIMITER = " ";

const char* TrieClassifier::type_ = "trie";

using namespace NQI;

TrieClassifier::TrieClassifier(ClassifierContext* context)
    : Classifier(context)
{
    NQI::KeyType name;
    name.name_ = context_->name_;
    keyPtr_ = context_->config_->find(name);
    synonym_.clear();
    loadSynonym();
    loadLexicon();
}

TrieClassifier::~TrieClassifier()
{
    trie_.clear();
}

void TrieClassifier::loadSynonym()
{
    std::string synonymFile = context_->lexiconDirectory_ + context_->name_ + "Synonym";
    if (!boost::filesystem::exists(synonymFile) 
        || !boost::filesystem::is_regular_file(synonymFile))
        return;
    std::ifstream fs;
    fs.open(synonymFile.c_str(), std::ios::in);
    if (!fs)
        return;
    std::string line;
    while (!fs.eof())
    {
        getline(fs, line);
        std::size_t found = line.find(FORMAT_SYNONYM);
        if (std::string::npos == found)
            continue;
        std::size_t pos = found + 1;
        MultiValueType mv;
        while (std::size_t synonym = line.find(SYNONYM_DELIMITER, pos))
        {
            if (std::string::npos == synonym)
            {
                mv.push_back(line.substr(pos, line.size() - pos));
                //std::cout<<line.substr(pos, line.size() - pos)<<std::endl;
                break;
            }
            mv.push_back(line.substr(pos, synonym - pos));
            //std::cout<<line.substr(pos, synonym - pos)<<std::endl;
            pos = synonym + 1;
        }
        line.erase(found, line.size() - found);
        line += " ";
        pos = 0;
        while (std::size_t synonym = line.find(SYNONYM_DELIMITER, pos))
        {
            std::string key;
            if (std::string::npos == synonym)
            {
                break;
            }
            key = line.substr(pos, synonym - pos);
            //std::cout<<key<<"\n";
            SynonymIterator it = synonym_.find(key);
            if (synonym_.end() == it)
                synonym_.insert(make_pair(key, mv));
            else
            {
                MultiVIterator mit = mv.begin();
                for (; mit != mv.end(); mit++)
                    it->second.push_back(*mit);
            }
            pos = synonym + 1;
        }
    }
}

void TrieClassifier::loadLexicon()
{
    std::string lexiconFile = context_->lexiconDirectory_ + context_->name_;
    if (!boost::filesystem::exists(lexiconFile) 
        || !boost::filesystem::is_regular_file(lexiconFile))
        return;
    std::ifstream fs;
    fs.open(lexiconFile.c_str(), std::ios::in);
    if (!fs)
        return;
    std::string line;
    short delimiters = 0;
    std::queue<std::string> words;
    while (!fs.eof())
    {
        getline(fs, line);
        boost::trim(line);
        if (FORMAT_LSB == line)
            delimiters++;
        else if (FORMAT_RSB == line)
            delimiters--;
        if (0 > delimiters)
        {
            break;
        }
        if (0 == delimiters)
        {
            std::stack<std::string> stack;
            std::string category = "";
            while (!words.empty())
            {
                std::string word = words.front();
                words.pop();
                if (FORMAT_LSB == word)
                {
                    stack.push(category);
                    continue;
                }
                else if(FORMAT_RSB == word)
                {
                    if (stack.empty())
                        break;
                    stack.pop();
                    continue;
                }
                else if(!stack.empty()) 
                {
                    category = stack.top();
                }

                if (category.empty())
                {
                    category = word;
                }
                else
                {
                    category += NODE_DELIMITER;
                    category += word;
                }
                TrieIterator it = trie_.find(word);
                if (trie_.end() == it)
                {
                    MultiValueType mv;
                    mv.push_back(category);
                    trie_.insert(make_pair(word, mv));
                }
                else
                    it->second.push_back(category);

                std::size_t pos = 0;
                while (std::size_t found = word.find(FORMAT_OR, pos))
                {
                    std::size_t size = 0;
                    if (std::string::npos == found)
                    {
                        size = word.size() - pos;
                    }
                    else
                    {
                        size = found - pos;
                    }
                    std::string key = word.substr(pos, size);
                    pos = found + 1;
                    //std::cout<<key<<std::endl;
                    SynonymIterator it = synonym_.find(key);
                    if (synonym_.end() == it)
                    {
                        MultiValueType mv;
                        mv.push_back(word);
                        synonym_.insert(make_pair(key, mv));
                    }
                    else
                    {
                        it->second.push_back(word);
                    }
                    if (std::string::npos == found)
                        break;
                }
                //std::cout<<word<<" "<< category<<"\n";
            }
        }
        words.push(line);
    }
    fs.close();
    return;
}

void TrieClassifier::reloadLexicon()
{
    boost::unique_lock<boost::shared_mutex> ul(mtx_);
    loadLexicon();
}

bool TrieClassifier::classify(WMVContainer& wmvs, std::string& query)
{
    if (query.empty())
        return false;
    
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);
    if (!sl)
    {
        std::cout<<"shared_lock::try_lock return'\n";
        return false;
    }
    
    std::size_t pos = 0;
    bool ret = false;
    bool isBreak = false;
   
    // exact search and erase
    WMVType& mvContainer = wmvs[*keyPtr_];
    //std::queue<std::string> voters;
    std::list<std::string> voters;
    std::string removedWords = "";
    while ((pos < query.size()) && (!isBreak))
    {
        std::size_t found = query.find(KEYWORD_DELIMITER, pos);
        std::size_t size = 0;
        if (std::string::npos == found)
        {
            size = query.size() - pos;
            isBreak = true;
        }
        else
        {
            size = found - pos;
        }
        std::string word = query.substr(pos, size);

        std::list<std::string>::iterator voter = voters.begin();
        for (; voter != voters.end(); voter++)
        {
            if (*voter == word)
            {
                voters.push_back(word);
                if (trie_.end() != trie_.find(word))
                {
                    query.erase(pos, size + 1);
                    removedWords += " ";
                    removedWords += word;
                    ret = true;
                }
                break;
            }
        }
        if (voters.end() != voter)
        {
            pos = found+1;
            continue;
        }

        TrieIterator it = trie_.find(word);
        if (trie_.end() != it)
        {
            MultiValueType& mv = it->second;
            MultiVIterator vIt = mv.begin();
            for (; vIt != mv.end(); vIt++)
                mvContainer.push_back(make_pair(*vIt,0));
            
            voters.push_back(word);
            query.erase(pos, size + 1);
            removedWords += " ";
            removedWords += word;
            ret = true;
        }
        else
        {
            // synonym. fuzzy search and reserve.
            SynonymIterator sit = synonym_.find(word);
            if (synonym_.end() != sit)
            {
                MultiVIterator mvIt = sit->second.begin();
                for (; mvIt != sit->second.end(); mvIt++)
                {
                    std::list<std::string>::iterator voter = voters.begin();
                    for (; voter != voters.end(); voter++)
                    {
                        if (*voter == *mvIt)
                        {
                            voters.push_back(*mvIt);
                            break;
                        }
                    }
                    if (voters.end() != voter)
                        continue;
                    
                    voters.push_back(*mvIt);
                    TrieIterator tIt = trie_.find(*mvIt);
                    if (trie_.end() != tIt)
                    {
                        MultiValueType& mv = tIt->second;
                        MultiVIterator vIt = mv.begin();
                        for (; vIt != mv.end(); vIt++)
                            mvContainer.push_back(make_pair(*vIt,0));
                        ret = true;
                    }
                }
            }
            else
            {
                voters.push_back(word);
            }
            pos = found+1;
        }
    }
    
    //std::cout<<"vote begin\n";
    std::list<std::string>::iterator vIt = voters.begin();
    for (; vIt != voters.end(); vIt++)
    {
        std::string word = *vIt;
        //std::cout<<word<<"\n"; 
        // vote
        // 男=>服饰鞋帽>男装>...
        // 保护套=>手机数码>手机配件>手机保护套
        WMVIterator wmvIt = mvContainer.begin();
        for (; wmvIt != mvContainer.end(); wmvIt++)
        {
            calculateWeight(*wmvIt, word);
            //std::cout<<wmvIt->first<<" "<<wmvIt->second<<"\n";
        }
    }
    voters.clear();
    //std::cout<<query<<"\n"<<removedWords<"\n";
    //std::cout<<"vote end\n";
    reserveKeywords(mvContainer, removedWords);
    // combine.
    // query:男装 外套
    // 男装=>服饰鞋帽>男装
    // 外套=>服饰鞋帽>男装>外套
    //       服饰鞋帽>女装>外套
    // 男装 外套=>服饰鞋帽>男装>外套
    /*mvContainer.sort(nameCompare);
    WMVIterator it = mvContainer.begin();
    for (; it != mvContainer.end(); it++)
    {
        WVType& value = *it;
        WMVIterator cIt = it;
        cIt ++;
        for (; cIt != mvContainer.end(); cIt++)
        {
           calculateWeight(*cIt, value); 
        }
    }
    mvContainer.sort(weightCompare);
    unsigned int maxWeight = 0;
    for (it = mvContainer.begin(); it != mvContainer.end(); it++)
    {
        if (maxWeight > it->second)
            break;
        wmvs[*iCategory_].push_back(*it);
        maxWeight = it->second;
    }*/
    return ret;
}

}

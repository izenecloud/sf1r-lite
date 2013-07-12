#include "LexiconClassifier.h"
#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
//#include <boost/thread/lock_types.hpp>

#include <fstream>
#include <iostream>
//#include <sys/inotify.h>

namespace sf1r
{
const char* FORMAL_ALIAS_DELIMITER = ":";
const char* ALIAS_DELIMITER = ";";
const char* KEYWORD_DELIMITER = " ";

const char* LexiconClassifier::type_ = "lexicon";
using namespace NQI;

LexiconClassifier::LexiconClassifier(ClassifierContext* context)
    : Classifier(context)
{
    NQI::KeyType name;
    name.name_ = context_->name_;
    keyPtr_ = context_->config_->find(name);
    loadLexicon();
    //reloadThread_= boost::thread(&LexiconClassifier::reloadLexicon_, this);
}

LexiconClassifier::~LexiconClassifier()
{
    lexicons_.clear();
}

void LexiconClassifier::loadLexicon()
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
    while (!fs.eof())
    {
        getline(fs, line);
        std::size_t formalEnd = line.find(FORMAL_ALIAS_DELIMITER);
        if (std::string::npos == formalEnd)
        {
            unsigned short size= line.size();
            if (0 == size)
                continue;
            lexicons_.insert(make_pair(line, line));
        }
        else
        {
            std::string formalName = line.substr(0, formalEnd);
            unsigned short size= formalName.size();
            if (0 == size)
                continue;
            lexicons_.insert(make_pair(formalName, formalName));
                
            line.erase(0, formalEnd + 1);
            while (true)
            {
                std::size_t aliasEnd = line.find(ALIAS_DELIMITER);
                if (std::string::npos == aliasEnd)
                {
                    unsigned short size= line.size();
                    if (0 == size)
                        break;
                    lexicons_.insert(make_pair(line, formalName));
                    break;
                }
                std::string aliasName = line.substr(0, aliasEnd);
                line.erase(0, aliasEnd + 1);
                unsigned short size= aliasName.size();
                if (0 == size)
                    continue;
                lexicons_.insert(make_pair(aliasName, formalName));
            }
        }
    }
    fs.close();
    return;
}

void LexiconClassifier::reloadLexicon()
{
    boost::unique_lock<boost::shared_mutex> ul(mtx_);
    loadLexicon();
    /*const char* lexiconDirectory = context_->lexiconDirectory_.c_str();
    int ld = -1;
    if (-1 == (ld = inotify_add_watch(inotify_init(), lexiconDirectory, 
        IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)))
    {
        std::cout<<"Inotify Init Error\n";
        return;
    }
    struct inotify_event ievent;
    while (true)
    {
        int ret = read(ld, &ievent, sizeof(ievent));
        if ((-1 == ret) && (EINTR == errno))
            continue;
        if (IN_MODIFY == ievent.mask)
        {
            std::string fileName = ievent.name;
            QIIterator it = context_->config_->begin();
            for(; it != context_->config_->end(); it++)
            {
                if (fileName == it->name_)
                    break;
            }
            if (it != context_->config_->end())
            {
                boost::unique_lock<boost::shared_mutex> ul(mtx_);
                loadLexicon_();
            }
        }
    }*/
}

bool LexiconClassifier::classify(WMVContainer& wmvs, std::string& query)
{
    if (query.empty())
        return false;
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);
    if (!sl)
    {
        std::cout<<"shared_lock::try_lock return'\n";
        return -1;
    }
    if (lexicons_.empty() )
        return false;
    
    std::size_t pos = 0;
    std::size_t end = query.size();
    bool ret = false;
    bool isBreak = false;

    while ((pos < end) && (!isBreak))
    {
        std::size_t found = query.find_first_of(KEYWORD_DELIMITER, pos);
        std::size_t size = 0;
        if (std::string::npos == found)
        {
            size = end - pos;
            isBreak = true;
        }
        else
        {
            size = found - pos;
        }
        std::string word = query.substr(pos, size);
        
        boost::unordered_map<std::string, std::string>::iterator it = lexicons_.find(word);
        if (lexicons_.end() != it)
        {
            word = it->second;
            wmvs[*keyPtr_].push_back(make_pair(word, 1));
            query.erase(pos, found-pos+1);
            ret = true;
        }
        else
        {
            pos = found+1;
        }
    }

    return ret;

    /*size_t pos = 0;
    size_t end = query.size() - 1;
    unsigned short wordSize = maxLength_;
    while (pos <= end)
    {
        while (minLength_ <= wordSize)
        {
            if (end < pos + wordSize -1)
            {
                wordSize--;
                continue;
            }
            
            std::string word = query.substr(pos, wordSize);
            boost::unordered_map<std::string, std::string>::iterator it = lexicons_.find(word);
            if (lexicons_.end() != it)
            {
                word = it->second;
            }
            else
            {
                wordSize--;
                continue;
            }
            intents[iCategory_].push_back(word);
            query.erase(pos, wordSize);
            if (end <= wordSize)
                return true;
            end -= wordSize;
            wordSize = maxLength_;
        }
        if (minLength_ > wordSize)
        {
            pos++;
            wordSize = maxLength_;
        }
    }*/
}

}

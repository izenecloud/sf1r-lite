#include "LexiconClassifier.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
namespace sf1r
{
const char* FORMAL_ALIAS_DELIMITER = ":";
const char* ALIAS_DELIMITER = ";";
LexiconClassifier::LexiconClassifier(ClassifierContext& context)
    : Classifier(context)
{
    maxLength_ = 0;
    minLength_ = -1;
    loadLexicon_();
}

LexiconClassifier::~LexiconClassifier()
{
    lexicons_.clear();
}

void LexiconClassifier::loadLexicon_()
{
    QIIterator it = context_.config_.begin();
    for(; it != context_.config_.end(); it++)
    {
        QueryIntentCategory iCategory = (*it);
        std::string lexiconFile = context_.lexiconDirectory_ + iCategory.name_;
        if (!boost::filesystem::exists(lexiconFile) 
            || !boost::filesystem::is_regular_file(lexiconFile))
            continue;
        std::ifstream fs;
        fs.open(lexiconFile.c_str(), std::ios::in);
        if (!fs)
            continue;
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
                if ( maxLength_ < size )
                    maxLength_ = size;
                if ( minLength_ > size )
                    minLength_ = size;
                lexicons_[iCategory].insert(make_pair(line, line));
                std::cout<<line<<" "<< line<<"\n";
            }
            else
            {
                std::string formalName = line.substr(0, formalEnd);
                unsigned short size= formalName.size();
                if (0 == size)
                    continue;
                if ( maxLength_ < size )
                    maxLength_ = size;
                if ( minLength_ > size )
                    minLength_ = size;
                lexicons_[iCategory].insert(make_pair(formalName, formalName));
                std::cout<<formalName<<" "<< formalName<<"\n";
                
                line.erase(0, formalEnd + 1);
                while (true)
                {
                    std::size_t aliasEnd = line.find(ALIAS_DELIMITER);
                    if (std::string::npos == aliasEnd)
                    {
                        unsigned short size= line.size();
                        if (0 == size)
                            break;
                        if ( maxLength_ < size )
                            maxLength_ = size;
                        if ( minLength_ > size )
                            minLength_ = size;
                        lexicons_[iCategory].insert(make_pair(line, formalName));
                std::cout<<line<<" "<< formalName<<"\n";
                        break;
                    }
                    std::string aliasName = line.substr(0, aliasEnd);
                    line.erase(0, aliasEnd + 1);
                    unsigned short size= aliasName.size();
                    if (0 == size)
                        continue;
                    if ( maxLength_ < size )
                        maxLength_ = size;
                    if ( minLength_ > size )
                        minLength_ = size;
                    lexicons_[iCategory].insert(make_pair(aliasName, formalName));
                std::cout<<aliasName<<" "<< formalName<<"\n";
                }
            }
        }
        fs.close();
    }
    return;
}

bool LexiconClassifier::classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query)
{
    if ((0 == maxLength_) || lexicons_.empty())
        return false;
    size_t pos = 0;
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
            std::map<QueryIntentCategory, boost::unordered_map<std::string, std::string> >::iterator it = lexicons_.begin();
            for(; it != lexicons_.end(); it++)
            {
                boost::unordered_map<std::string, std::string>::iterator itAlais= it->second.find(word);
                if (it->second.end() != itAlais)
                {
                    word = itAlais->second;
                    break;
                }
            }
            if (it == lexicons_.end())
            {
                wordSize--;
                continue;
            }
            intents[it->first].push_back(word);
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
    }
    return intents.empty() ? false : true;
}

}

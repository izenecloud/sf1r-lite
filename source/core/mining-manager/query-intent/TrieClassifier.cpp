#include "TrieClassifier.h"
#include <boost/filesystem.hpp>

#include <fstream>
#include <queue>
#include <iostream>

namespace sf1r
{
const std::string FORMAT_LSB = "[";
const std::string FORMAT_RSB = "]";

const char* TrieClassifier::type_ = "trie";

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
                    category += ">";
                    category += word;
                }
                std::size_t size = word.size();
                if (size == 0)
                    continue;
                if (size > maxLength_)
                    maxLength_ = size;
                if (size < minLength_)
                    minLength_ = size;
                lexicons_.insert(make_pair(word, category));
                //std::cout<<word<<" "<< category<<"\n";
            }
        }
        words.push(line);
    }
    fs.close();
    return;
}

bool TrieClassifier::classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query)
{
    if (!LexiconClassifier::classify(intents, query))
        return false;
    if ( -1 == iCategory_.operands_)
        return true;
    if ((std::size_t)iCategory_.operands_ < intents[iCategory_].size())
    {
        if (iCategory_.operands_ == 1)
        {
            std::list<std::string> values = intents[iCategory_];
            std::list<std::string>::iterator it = values.begin();
            std::size_t max = 0;
            std::string maxString;
            for (; it != values.end(); it++)
            {
                if (max < it->size())
                    maxString = (*it);
            }
            intents[iCategory_].clear();
            intents[iCategory_].push_back(maxString);
        }
    }
    return true;
}

}

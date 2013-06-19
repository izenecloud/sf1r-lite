/**
 * @file TrieClassifier.h
 * @brief classify qyery string via Trie
 * @author Kevin Lin
 * @date Created 2013-06-19
 */

#ifndef SF1R_TRIE_CLASSIFIER_H
#define SF1R_TRIE_CLASSIFIER_H

#include "LexiconClassifier.h"

namespace sf1r
{

class TrieClassifier : public LexiconClassifier
{
public:
    TrieClassifier(ClassifierContext* context)
        : LexiconClassifier(context)
    {
    }
public:
    bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query);
    
    const char* name()
    {
        return context_->name_.c_str();
    }
    
    static const char* type()
    {
        return type_;
    }
    
    void loadLexicon();
private:
    static const char* type_;
};

}

#endif //TrieClassifier.h

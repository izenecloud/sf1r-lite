/**
 * @file TrieClassifier.h
 * @brief classify qyery string via Trie
 * @author Kevin Lin
 * @date Created 2013-06-19
 */

#ifndef SF1R_TRIE_CLASSIFIER_H
#define SF1R_TRIE_CLASSIFIER_H

#include "Classifier.h"

namespace sf1r
{

class TrieClassifier : public Classifier
{
typedef boost::unordered_map<std::string, NQI::MultiValueType> TrieContainer;
typedef TrieContainer::iterator TrieIterator;

typedef boost::unordered_map<std::string, NQI::MultiValueType> SynonymContainer;
typedef SynonymContainer::iterator SynonymIterator;

public:
    TrieClassifier(ClassifierContext* context);
    ~TrieClassifier();
public:
    bool classify(NQI::WMVContainer& wmvs, std::string& query);
    
    const char* name()
    {
        return context_->name_.c_str();
    }
    
    static const char* type()
    {
        return type_;
    }
    
    void loadLexicon();
    
    void loadSynonym();
 
    void reloadLexicon();

    int priority()
    {
        return 0;
    }
private:
    TrieContainer trie_;
    SynonymContainer synonym_;
    NQI::KeyTypePtr keyPtr_;
    boost::shared_mutex  mtx_;
    
    static const char* type_;
};

}

#endif //TrieClassifier.h

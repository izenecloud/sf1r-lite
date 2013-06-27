/**
 * @file LexiconClassifier.h
 * @brief classify qyery string via Lexicon
 * @author Kevin Lin
 * @date Created 2013-06-07
 */

#ifndef SF1R_LEXICON_CLASSIFIER_H
#define SF1R_LEXICON_CLASSIFIER_H

#include "Classifier.h"
#include <boost/unordered_map.hpp>
#include <boost/thread/thread.hpp>

namespace sf1r
{

class LexiconClassifier : public Classifier
{
public:
    LexiconClassifier(ClassifierContext* context);
    ~LexiconClassifier();
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
    
    void reloadLexicon();
    void loadLexicon();

    int priority()
    {
        return 0;
    }
private:
    boost::unordered_map<std::string, std::string> lexicons_;
    unsigned short maxLength_;
    unsigned short minLength_;
    NQI::KeyTypePtr keyPtr_;
    //std::map<QueryIntentCategory, boost::unordered_map<std::string, std::string> >lexicons_;
    static const char* type_;

    //boost::thread reloadThread_;
    boost::shared_mutex  mtx_;
};

}

#endif //SourceClassifier.h

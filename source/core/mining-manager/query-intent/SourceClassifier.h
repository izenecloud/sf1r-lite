/**
 * @file SourceClassifier.h
 * @brief classify qyery string via Lexicon
 * @author Kevin Lin
 * @date Created 2013-06-07
 */

#ifndef SF1R_SOURCE_CLASSIFIER_H
#define SF1R_SOURCE_CLASSIFIER_H

#include "Classifier.h"
#include <boost/unordered_map.hpp>
#include <boost/thread/thread.hpp>

namespace sf1r
{

class SourceClassifier : public Classifier
{
public:
    SourceClassifier(ClassifierContext* context);
    ~SourceClassifier();
public:
    bool classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query);
    const char* name()
    {
        return name_;
    }
    static const char* type()
    {
        return name_;
    }
    void reload();
private:
    void loadLexicon_();
private:
    //std::map<QueryIntentCategory, boost::unordered_map<std::string, std::string> >lexicons_;
    boost::unordered_map<std::string, std::string> lexicons_;
    unsigned short maxLength_;
    unsigned short minLength_;
    QueryIntentCategory iCategory_;
    static const char* name_;

    //boost::thread reloadThread_;
    boost::shared_mutex  mtx_;
};

}

#endif //SourceClassifier.h

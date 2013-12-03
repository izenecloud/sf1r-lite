/**
 * @file Classifier.h
 * @brief Interface for query classify.
 * @author Kevin Lin
 * @date Created 2013-06-05
 */

#ifndef SF1R_CLASSIFIER_H
#define SF1R_CLASSIFIER_H

#include <map>
#include <list>
#include <string>
#include <utility>

#include <configuration-manager/QueryIntentConfig.h>
#include "../MiningManager.h"

namespace sf1r
{

typedef std::string ClassifierType;
typedef std::string ClassifierName;

extern const char* KEYWORD_DELIMITER;

namespace NQI   //NamespaceQueryIntent
{
    extern const unsigned int REMOVED_WORD_WEIGHT;
    extern const std::string NODE_DELIMITER;

    typedef QueryIntentCategory KeyType;
    typedef QIIterator KeyTypePtr;
    typedef std::string ValueType;
    typedef std::list<ValueType>   MultiValueType;
    typedef MultiValueType::iterator MultiVIterator;
    
    typedef std::pair<ValueType, unsigned int> WVType;
    typedef std::list<WVType >   WMVType; // Weighted multivalue type
    typedef WMVType::iterator WMVIterator;

    typedef std::map<KeyType, WMVType> WMVContainer; // Weighted multivalue type container
    typedef WMVContainer::iterator WMVCIterator;
    
    bool nameCompare(WVType& lv, WVType& rv);
    bool weightCompare(WVType& lv, WVType& rv);
    
    bool calculateWeight(WVType& lv, WVType& rv);
    bool calculateWeight(WVType& wv, ValueType& v);
    
    //void combineWMVS(WMVContainer& wmvs, std::string& remainKeywords);
    void combineWMVS(WMVContainer& wmvs, boost::unordered_map<std::string, std::list<std::string> >& scs, std::string& remainKeywords);
    bool combineWMV(WMVType& wmv, int number);
    bool commonAncester(WMVType& wmv, int number);
    void printWMV(WMVType& wmv);

    void reserveKeywords(WMVType& wmv, std::string& removedWords);
    void resumeKeywords(WMVType& wmv, std::string& keyword);
    void deleteKeywords(WMVType& wmv);

} // namespace NQI

class ClassifierContext
{
public:
    ClassifierContext(QueryIntentConfig* config, std::string directory, 
        MiningManager* miningManager,ClassifierType type, ClassifierName name)
        : config_(config)
        , lexiconDirectory_(directory)
        , miningManager_(miningManager)
        , type_(type)
        , name_(name)
    {
    }
public:
    QueryIntentConfig* config_;
    std::string lexiconDirectory_;
    MiningManager* miningManager_;
    ClassifierType type_;
    ClassifierName name_;
};

class Classifier
{
public:
    Classifier(ClassifierContext* context)
        : context_(context)
    {
    }
    virtual ~Classifier()
    {
        if (context_)
        {
            delete context_;
            context_ = NULL;
        }
    }
public:
    //
    // classify query into several QueryIntentType, remove classified keywords from query.
    //
    virtual bool classify(NQI::WMVContainer& wmvs, std::string& query) = 0;

    virtual const char* name() = 0;

    virtual int priority() = 0;

    virtual void reloadLexicon()
    {
    }

    static const char* Type()
    {
        return NULL;
    }
protected:
    ClassifierContext* context_;
};

}

#endif //Classifier.h

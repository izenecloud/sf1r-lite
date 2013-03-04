#ifndef SF1R_CMA_KNOWLEDGE_FACTORY_H
#define SF1R_CMA_KNOWLEDGE_FACTORY_H

#include <icma/icma.h>
#include <util/singleton.h>

#include <map>

namespace sf1r{

class CMAKnowledgeFactory
{
    std::map<std::string, cma::Knowledge*> knowledges_;
public:
    CMAKnowledgeFactory(){}
    ~CMAKnowledgeFactory()
    {
        for(std::map<std::string, cma::Knowledge*>::iterator kit = knowledges_.begin();
               kit != knowledges_.end(); ++kit)
            delete kit->second;
    }

    static CMAKnowledgeFactory* Get()
    {
        return izenelib::util::Singleton<CMAKnowledgeFactory>::get();
    }

    cma::Knowledge* GetKnowledge(const std::string& knowledgePath, bool toLoadModel = true) 
    {
        std::map<std::string, cma::Knowledge*>::iterator kit = knowledges_.find(knowledgePath);
        if( kit != knowledges_.end())  return kit->second;
        else
        {
            cma::Knowledge* knowledge = cma::CMA_Factory::instance()->createKnowledge();
            knowledge->loadModel( "utf8", knowledgePath.c_str(), toLoadModel);
            knowledges_[knowledgePath] = knowledge;
            return knowledge;
        }
    }

};

}

#endif


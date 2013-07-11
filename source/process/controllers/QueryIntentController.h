#ifndef PROCESS_CONTROLLERS_QUERY_INTENT_CONTROLLER_H
#define PROCESS_CONTROLLERS_QUERY_INTENT_CONTROLLER_H
/**
 * @file QueryIntentController.h
 * @author Kevin Lin
 * @date Created 2013-06-09
 */
#include "Sf1Controller.h"

namespace sf1r
{
class MiningSearchService;

class QueryIntentController : public Sf1Controller
{
public:
    QueryIntentController();

public:
    void reload();
protected:
    virtual bool checkCollectionService(std::string& error);

private:
    MiningSearchService* miningSearchService_;
};


} // namespace sf1r

#endif //QueryIntentController.h 



#ifndef PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H
#define PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H
/**
 * @file process/controllers/KeywordsController.h
 * @author Ian Yang
 * @date Created <2010-06-01 16:03:32>
 */
#include "Sf1Controller.h"

namespace sf1r
{
class MiningSearchService;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b keywords
 *
 * Gets list of keywords.
 */
class KeywordsController : public Sf1Controller
{
public:
    KeywordsController();

    enum
    {
        kDefaultCount = 8       /**< Default count of result */
    };

    void index();

    void inject_query_correction();

    void inject_query_recommend();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    MiningSearchService* miningSearchService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_KEYWORDS_CONTROLLER_H



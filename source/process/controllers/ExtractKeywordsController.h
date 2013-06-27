#ifndef PROCESS_CONTROLLERS_EXTRACTKEYWORDS_CONTROLLER_H
#define PROCESS_CONTROLLERS_EXTRACTKEYWORDS_CONTROLLER_H
/**
 * @file process/controllers/ExtractKeywordsController.h
 * @author Kuilong Liu
 * @date Created <2013-06-26 13:28:32>
 */
#include "Sf1Controller.h"
#include <b5m-manager/product_matcher.h>
#include <string>

namespace sf1r
{
class ProductMatcher;

/**
 * @brief Controller \b extract keywords
 *
 * Extract keywords from text.
 */
class ExtractKeywordsController : public ::izenelib::driver::Controller
{
public:
    ExtractKeywordsController();

    void extract_keywords();

private:
    ProductMatcher* matcher_;
};

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_EXTRACTKEYWORDS_CONTROLLER_H



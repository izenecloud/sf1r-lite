#include "QueryCorrectionController.h"

#include <common/Keys.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;

QueryCorrectionController::QueryCorrectionController()
    :queryLogSearchService_(NULL)
{}

QueryCorrectionController::QueryCorrectionController(
    const QueryCorrectionController& controller
)
    :queryLogSearchService_(controller.queryLogSearchService_)
{
}

void QueryCorrectionController::index()
{
    std::string collectionName = asString(request()[Keys::collection]);
    std::string queryString = asString( request()[Keys::keywords] );

    UString queryUString(queryString, UString::UTF_8);
    UString refinedQueryString;

    queryLogSearchService_->getRefinedQuery(
        collectionName, queryUString,
        refinedQueryString);

    std::string convertBuffer;
    refinedQueryString.convertString(convertBuffer,
                                     izenelib::util::UString::UTF_8);
    response()[Keys::refined_query] = convertBuffer;
}

} // namespace sf1r

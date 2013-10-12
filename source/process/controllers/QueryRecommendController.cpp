#include "QueryRecommendController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>
#include <common/CollectionManager.h>
#include <mining-manager/query-recommendation/RecommendEngineWrapper.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;

QueryRecommendController::QueryRecommendController()
    : Sf1Controller(false)
{
}

bool QueryRecommendController::checkCollectionService(std::string& error)
{
    return false;
}

void QueryRecommendController::recommend()
{
    std::string queryString = asString(request()[Keys::keywords]);
    uint32_t N = asInt(request()["recommend_num"]);
    
    std::vector<std::string> results;
    RecommendEngineWrapper::getInstance().recommend(queryString, N, results);
    for (std::size_t i = 0; i < results.size(); i++)
    {
        response()["recommend_queries"]() = results[i];
    }   
}

} // namespace sf1r

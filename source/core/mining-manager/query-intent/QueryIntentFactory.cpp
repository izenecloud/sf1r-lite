#include "QueryIntentFactory.h"
#include "ProductQueryIntent.h"

namespace sf1r
{

QueryIntent* QueryIntentFactory::createQueryIntent(QueryIntentContext& context)
{
    return new ProductQueryIntent();
}

}

#include "QueryIntentFactory.h"
#include "ProductQueryIntent.h"

namespace sf1r
{

QueryIntent* QueryIntentFactory::createQueryIntent(IntentContext* context)
{
    return new ProductQueryIntent(context);
}

}

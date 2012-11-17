#include "ProductScoreConfig.h"
#include <sstream>

using namespace sf1r;

ProductScoreConfig::ProductScoreConfig()
    : type(PRODUCT_SCORE_NUM)
    , weight(0)
{}

std::string ProductScoreConfig::toStr() const
{
    std::ostringstream oss;

    oss << "type: " << type << ", ";

    if (!propName.empty())
    {
        oss << "property: " << propName << ", ";
    }

    oss << "weight: " << weight << std::endl;

    if (!factors.empty())
    {
        for (std::size_t i = 0; i < factors.size(); ++i)
        {
            oss << "factor: " << i << ", "
                << factors[i].toStr();
        }
    }

    return oss.str();
}

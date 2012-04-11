#ifndef SF1R_PRODUCTMANAGER_PMTYPES_H
#define SF1R_PRODUCTMANAGER_PMTYPES_H

#include <common/type_defs.h>
#include <document-manager/Document.h>

namespace sf1r
{

typedef double ProductPriceType;
typedef Document PMDocumentType;

typedef std::vector<std::pair<time_t, std::pair<ProductPriceType, ProductPriceType> > > PriceHistoryItem;
typedef std::vector<std::pair<uint128_t, PriceHistoryItem> > PriceHistoryList;
typedef std::vector<std::pair<uint128_t, std::pair<ProductPriceType, ProductPriceType> > > PriceRangeList;

typedef std::vector<std::pair<float, uint128_t> > TPCQueue;

struct ProductEditOption
{
//  bool backup;
    bool force;
//  bool ignore_loss;

    ProductEditOption()
        : force(false)
    {
    }
};

}

MAKE_MEMCPY_TYPE(sf1r::TPCQueue)

#endif

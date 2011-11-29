#ifndef SF1R_PRODUCTMANAGER_PMTYPES_H
#define SF1R_PRODUCTMANAGER_PMTYPES_H

#include <common/type_defs.h>
#include <document-manager/Document.h>

namespace sf1r
{

typedef double ProductPriceType;
typedef Document PMDocumentType;

typedef std::vector<std::pair<std::string, std::pair<ProductPriceType, ProductPriceType> > > PriceHistoryItem;
typedef std::vector<std::pair<std::string, PriceHistoryItem> > PriceHistoryList;
typedef std::vector<std::pair<std::string, std::pair<ProductPriceType, ProductPriceType> > > PriceRangeList;

typedef std::vector<std::pair<float, std::string> > TPCQueue;

}

#endif

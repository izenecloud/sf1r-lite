/**
 * \file ProductInfo.h
 * \brief
 * \date Sep 9, 2011
 * \author Xin Liu
 */

#ifndef _PRODUCT_INFO_H_
#define _PRODUCT_INFO_H_

#include "CassandraColumnFamily.h"

namespace sf1r {

class ProductInfo : public CassandraColumnFamily
{
public:
    ProductInfo() : CassandraColumnFamily() {}

    ~ProductInfo() {}

    DECLARE_COLUMN_FAMILY_COMMON_ROUTINES
};

}
#endif /*_PRODUCT_INFO_H_ */

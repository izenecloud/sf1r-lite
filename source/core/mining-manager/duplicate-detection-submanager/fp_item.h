#ifndef SF1R_DD_FPITEM_H_
#define SF1R_DD_FPITEM_H_

#include "DupTypes.h"
#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>

namespace sf1r
{
class FpItem
{
public:
    FpItem() : docid(0), length(0)
    {
    }

    FpItem(uint32_t pdocid, uint32_t plength, const std::vector<uint64_t>& pfp)
        : docid(pdocid), length(plength), fp(pfp)
    {
    }

    bool operator<(const FpItem& other) const
    {
        return docid < other.docid;
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & docid & length & fp ;
    }

public:
    uint32_t docid;
    uint32_t length;
    std::vector<uint64_t> fp;
};

}

#endif

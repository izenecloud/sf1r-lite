
#ifndef SF1R_DD_FPITEM_H_
#define SF1R_DD_FPITEM_H_


#include "DupTypes.h"
#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
namespace sf1r{
class FpItem
{
public:
  FpItem():docid(0), fp(), length(0)
  {
  }
  
  FpItem(uint32_t pdocid, const izenelib::util::CBitArray& pfp, uint32_t plength)
  :docid(pdocid), fp(pfp), length(plength)
  {
  }
  uint32_t docid;
  izenelib::util::CBitArray fp;
  uint32_t length;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
      ar & docid & fp & length;
  }
};
  
}

#endif

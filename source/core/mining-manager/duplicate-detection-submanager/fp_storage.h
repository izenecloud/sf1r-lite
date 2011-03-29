
#ifndef SF1R_DD_FPSTORAGE_H_
#define SF1R_DD_FPSTORAGE_H_


#include "DupTypes.h"
#include "fp_item.h"
#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
namespace sf1r{
class FpStorage
{
public:
  FpStorage(const std::string& dir);
  
  bool GetFpByTableId(uint32_t id, std::vector<FpItem>& fp);
  
  bool SetFpByTableId(uint32_t id, const std::vector<FpItem>& fp);

private:
  std::string dir_;
};
  
}

#endif

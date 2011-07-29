
#ifndef SF1R_ENUPDATABLEDICT_H_
#define SF1R_ENUPDATABLEDICT_H_
#include <stdint.h>
#include <la/dict/UpdatableDict.h>
namespace sf1r
{
class EkQueryCorrection;
 
class EnUpdatableDict : public la::UpdatableDict
{
public:
    EnUpdatableDict(EkQueryCorrection* ekc);
    
    
    virtual int update( const char* path, uint32_t last_modify );

    
private:
    EkQueryCorrection* ekc_;
    uint32_t previous_modify_;
    bool set_;

};

}
#endif


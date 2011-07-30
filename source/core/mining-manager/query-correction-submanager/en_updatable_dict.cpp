#include "en_updatable_dict.h"
#include "EkQueryCorrection.h"
using namespace sf1r;

EnUpdatableDict::EnUpdatableDict(EkQueryCorrection* ekc)
:ekc_(ekc), previous_modify_(0), set_(false)
{
}
    
    
int EnUpdatableDict::update( const char* path, uint32_t last_modify )
{
    std::cout<<"Start reload EN dictionary for correction."<<std::endl;
    if(!ekc_->ReloadEnResource())
    {
        std::cout<<"Reload EN Resource failed"<<std::endl;
        return 1;
    }
    std::cout<<"Reload EN Resource finished"<<std::endl;
    return 0;
//     std::cout<<"EnUpdatableDict::update"<<std::endl;
//     if(!set_)
//     {
//         previous_modify_ = last_modify;
//         set_ = true;
//         return 1;
//     }
//     if( last_modify!= previous_modify_)
//     {
//         std::cout<<"Start reload EN dictionary for correction."<<std::endl;
//         if(!ekc_->ReloadEnResource())
//         {
//             std::cout<<"Reload EN Resource failed"<<std::endl;
//         }
//         return 0;
//     }
//     return 1;
}


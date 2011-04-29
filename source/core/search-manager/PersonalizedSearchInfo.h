/**
 * @file PersonalizedSearchInfo.h
 * @author Zhongxia Li
 * @date Apr 29, 2011
 * @brief Wrap up Personalized Search Info
 */
#ifndef PERSONALIZED_SEARCH_INFO_H
#define PERSONALIZED_SEARCH_INFO_H

#include <recommend-manager/User.h>

namespace sf1r
{

struct PersonalSearchInfo
{
    User user;
    bool enabled;
};

}

#endif /* PERSONALIZED_SEARCH_INFO_H */

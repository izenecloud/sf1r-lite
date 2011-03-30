/*
 * MinuteEditDistance.h
 *
 * Copyright (C) 2007 KoreaWISEnut Inc. All Rights Reserved.
 *
 * This proprietary source code is owned by KoreaWISEnut Inc.
 * You are not allowed to copy, modify, adapt, translate, reverse engineer,
 * decompile, disassemble or otherwise attempt to discover the source code
 * of the software, except as expressly permitted by the given authority
 * through the agreements of KoreaWISEnut and the law in effect in
 * jurisdiction.
 *
 * @author: Park, Jeahyun (IM&S Team)
 */

#ifndef __MINUTE_EDIT_DISTANCE_H_
#define __MINUTE_EDIT_DISTANCE_H_


namespace sf1r
{
class MinuteEditDistance
{
public:
    static unsigned int getDistance(const char* s1, const char* s2);

private:
    MinuteEditDistance(void);

    static unsigned int min(unsigned int v1, unsigned int v2);
};
}
#endif/*__MINUTE_EDIT_DISTANCE_H_*/


/**
 * @file SynchroData.h
 * @author Zhongxia Li
 * @date Dec 7, 2011
 * @brief 
 */
#ifndef SYNCHRO_DATA_H_
#define SYNCHRO_DATA_H_

#include <util/kv2string.h>

namespace sf1r{

class SynchroData : public izenelib::util::kv2string
{
public:
    const static char* KEY_HOST;
    const static char* KEY_DATA_TYPE;
    const static char* KEY_DATA_PATH;
    const static char* KEY_COLLECTION;
    const static char* KEY_RETURN;
    const static char* KEY_ERROR;
};

}

#endif /* SYNCHRO_DATA_H_ */

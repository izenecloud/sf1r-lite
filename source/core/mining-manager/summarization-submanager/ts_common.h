/*
 * ts_common.h
 *
 *  Created on: 2010-10-15
 *      Author: jay
 */

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_TS_COMMON_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_TS_COMMON_H_

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <fcntl.h>
#include <cmath>
#include <stdlib.h>
#include <assert.h>

#include <ext/hash_map>
#include <ext/hash_set>

namespace __gnu_cxx{

template<>
struct hash<std::string>
{
    size_t
    operator()(const std::string& __s) const
    {
        return __stl_hash_string(__s.data());
    }
};

}

#endif /* TS_COMMON_H_ */

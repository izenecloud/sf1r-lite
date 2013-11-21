#ifndef SF1R_B5MMANAGER_B5MMODE_H_
#define SF1R_B5MMANAGER_B5MMODE_H_
#include "b5m_types.h"
#include <string>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <common/ScdParser.h>
#include <common/Utilities.h>

NS_SF1R_B5M_BEGIN

struct B5MMode
{
    enum {INC = 0, REBUILD, REMATCH, RETRAIN};
};

NS_SF1R_B5M_END

#endif


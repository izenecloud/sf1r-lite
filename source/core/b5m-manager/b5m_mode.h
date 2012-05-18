#ifndef SF1R_B5MMANAGER_B5MMODE_H_
#define SF1R_B5MMANAGER_B5MMODE_H_
#include <string>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <common/ScdParser.h>
#include <common/Utilities.h>

namespace sf1r {

    struct B5MMode
    {
        enum {INC = 0, REBUILD, REMATCH, RETRAIN};
    };


}

#endif


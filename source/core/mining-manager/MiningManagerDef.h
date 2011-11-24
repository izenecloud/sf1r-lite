
/// @file MiningManagerDef.h
/// @date 2009-08-19
/// @author Jia Guo
/// @brief This class defined sth. useful for mining manager and its sub manager.
///
/// @details
/// - Log

#ifndef _MINING_MANAGER_DEF_H_
#define _MINING_MANAGER_DEF_H_

#include <common/type_defs.h>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <string>
#include <iostream>
#include <3rdparty/febird/io/DataIO.h>
#include <3rdparty/febird/io/StreamBuffer.h>
#include <3rdparty/febird/io/FileStream.h>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <am/tokyo_cabinet/tc_hash.h>
#include <am/sdb_storage/sdb_storage.h>
#include <util/ustring/algo.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include "util/TermUtil.hpp"
#include <la-manager/LAManager.h>
#include <ir/ir_database/IRDatabase.hpp>
#include <limits>
#include "taxonomy-generation-submanager/TgTypes.h"
#include "MiningException.hpp"
namespace sf1r
{
typedef izenelib::ir::irdb::PureIRDatabase  TIRDatabase;
typedef izenelib::ir::irdb::PureIRDocument  TIRDocument;

}
#endif

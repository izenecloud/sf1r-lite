
/// @file MiningManagerDef.h
/// @date 2009-08-19
/// @author Jia Guo
/// @brief This class defined sth. useful for mining manager and its sub manager.
///
/// @details
/// - Log

#ifndef _MINING_MANAGER_DEF_H_
#define _MINING_MANAGER_DEF_H_

#include <ir/id_manager/IDManager.h>
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

typedef izenelib::ir::idmanager::_IDManager<izenelib::util::UString, uint32_t,
izenelib::util::NullLock, izenelib::ir::idmanager::EmptyWildcardQueryHandler<
izenelib::util::UString, uint32_t>,
izenelib::ir::idmanager::HashIDGenerator<izenelib::util::UString, uint32_t>,
izenelib::ir::idmanager::EmptyIDStorage<izenelib::util::UString, uint32_t>,
izenelib::ir::idmanager::EmptyIDGenerator<izenelib::util::UString, uint32_t>,
izenelib::ir::idmanager::EmptyIDStorage<izenelib::util::UString, uint32_t> >
MiningStrIDManager;

//     typedef izenelib::ir::idmanager::_IDManager<izenelib::util::UString, uint32_t,
//     izenelib::util::NullLock,
//     izenelib::ir::idmanager::EmptyWildcardQueryHandler<
//     izenelib::util::UString, uint32_t>,
//     izenelib::ir::idmanager::HashIDGenerator<izenelib::util::UString, uint32_t>,
//     izenelib::ir::idmanager::EmptyIDStorage<izenelib::util::UString, uint32_t>,
//     izenelib::ir::idmanager::UniqueIDGenerator<izenelib::util::UString, uint32_t>,
//     izenelib::ir::idmanager::TCIDStorage<izenelib::util::UString, uint32_t> > DocIDManager;

typedef izenelib::ir::idmanager::IDManager DocIDManager;


// class IDManagerForQS : public boost::noncopyable
// {
//     public:
//         IDManagerForQS()
//         {
//         }
//
//         ~IDManagerForQS()
//         {
//         }
//
//
//
//         void getAnalysisTermIdList(const izenelib::util::UString& str, std::vector<uint32_t>& termIdList)
//         {
//
//             TermUtil::getAnalysisTermIdListForRecommend(str, termIdList);
//         }
//
//         void getAnalysisTermIdList(const izenelib::util::UString& str, std::vector<izenelib::util::UString>& strList, std::vector<uint32_t>& termIdList)
//         {
//
//             TermUtil::getAnalysisTermIdListForRecommend( str, strList, termIdList);
//         }
//
//
//
//     private:
//         MiningIDManager* miningIdManager_;
//
// };

}
#endif

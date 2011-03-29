/*
 * LabelGroup.h
 *
 *  Created on: 2010-1-8
 *      Author: jinglei
 */

#ifndef GROUPINFO_H_
#define GROUPINFO_H_

#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem.hpp>
#include <common/type_defs.h>
#include <mining-manager/MiningManagerDef.h>
#include "LabelIDManager.hpp"
#include "TgTypes.h"

#include <am/3rdparty/rde_hash.h>
#include <ir/dup_det/group_table.hpp>
#include <hdb/HugeDB.h>
#include <am/sdb_hash/sdb_fixedhash.h>

#include <fstream>
#include <vector>
#include <string>

namespace sf1r
{

class GroupInfo
{
public:
    GroupInfo(const std::string& strPath, LabelIDManager*& labelIdManager);
    virtual ~GroupInfo();

    bool addForwardChunk(const std::vector<izenelib::util::UString>& wordSeq);

    bool addBackwardChunk(const std::vector<izenelib::util::UString>& wordSeq);

    bool groupLabels();

    bool addSimilarWordPair(uint32_t wordId1, uint32_t wordId2);

    bool loadGroupInfo();

    bool groupWordPair(uint16_t threshold=3);

    bool isWordSimilar(uint32_t wordId1, uint32_t wordId2, uint16_t& weight);

    void flush();
    void release();

private:
    /**
     * @ search the maximum matching segment from a text sequence against the label dictionary in a forward way.
     */
    bool minForwardMatching(const std::vector<izenelib::util::UString>& wordSeq, uint32_t labelId);

    /**
     * @ search the maximum matching segment from a text sequence against the label dictionary in a backward way.
     */
    bool minBackwardMatching(const std::vector<izenelib::util::UString>& wordSeq, uint32_t labelId);

    bool addSimilarLabelPair(uint32_t labelId1, uint32_t labelId2);

    bool makeWordPair(uint32_t wordId1,
            uint32_t wordId2,
            std::pair<uint32_t, uint32_t>& wordPair);
    bool makeWordPair(uint32_t wordId1,
            uint32_t wordId2,
            TgWordPairKey& wordPair);


private:

    typedef std::pair<uint32_t, uint32_t> pair_type;
    typedef std::pair<char, uint16_t> tag_type;
    typedef izenelib::util::NullLock lock_type;
    typedef izenelib::am::sdb_btree<TgWordPairKey, tag_type, lock_type>  container_type;
    typedef izenelib::hdb::HugeDB<
        TgWordPairKey,
        uint16_t,
        lock_type,
        container_type,
        true
    > HDBType;
    typedef izenelib::am::sdb_fixedhash<pair_type, uint16_t> INT_FIXED_HASH;

private:
    GroupInfo() {}
    std::string strPath_;
    LabelIDManager* labelIdManager_;
    izenelib::ir::GroupTable<>* labelGroupTable_;
    HDBType* wordPairTable_;
//    izenelib::am::rde_hash<key_type, uint16_t> similarWordPairCache_;
    INT_FIXED_HASH* similarWordPairCache_;
//    izenelib::ir::GroupTable<>* similarWordPairCache_;


    boost::shared_mutex rwMutex_;



};

}

#endif /* GROUPINFO_H_ */

///
/// @file TgTypes.h
/// @brief Some type definitions for TG.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-05-19
/// @date Updated 2009-11-21 01:04:09
///

#ifndef TGTYPES_H_
#define TGTYPES_H_


#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <ext/hash_map>
#include <cmath>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <util/ustring/UString.h>

#include <document-manager/UniqueDocIdentifier.h>
#include <am/cccr_hash/cccr_hash.h>
#include <idmlib/keyphrase-extraction/fwd.h>

#include <boost/dynamic_bitset.hpp>
#include <functional>

#include <boost/foreach.hpp>
#include <iostream>
#include <la/stem/Stemmer.h>
#include <am/tokyo_cabinet/tc_hash.h>
#include <am/sdb_hash/sdb_fixedhash.h>
#include <am/sdb_btree/sdb_btree.h>
#include <common/type_defs.h>
#include <util/izene_serialization.h>
#include <idmlib/concept-clustering/cc_types.h>
#include <3rdparty/msgpack/msgpack.hpp>
namespace sf1r
{

#define TG_VAR_DEF(var,path) var(path+"/"+#var+".tg")
typedef std::pair<izenelib::util::UString, std::vector<uint32_t> > ne_item_type;

struct ne_result_type
{
    izenelib::util::UString type_;
    std::vector<ne_item_type > itemList_;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type_;
        ar & itemList_;
    }
    DATA_IO_LOAD_SAVE(ne_result_type, &type_&itemList_);

    MSGPACK_DEFINE(type_,itemList_);
};

typedef std::vector<ne_result_type> ne_result_list_type;
typedef std::vector<termid_t> TIL;
typedef idmlib::cc::ClusterRep TgClusterRep;

// typedef idmlib::util::IDMTerm TgTerm;
//
//
//
//
// class TgDocStatus
// {
//     public:
//         bool applied_;
//         char status_;
//         docid_t docId_;
//         friend class boost::serialization::access;
//         template<class Archive>
//         void serialize(Archive & ar, const unsigned int version)
//         {
//             ar & applied_;
//             ar & status_;
//             ar & docId_;
//         }
// };
//
// typedef uint32_t TgLabelId;
//
// class TgLabelCache
// {
//     public:
//         std::vector<termid_t> termIdList_;
//
//         friend class boost::serialization::access;
//         template<class Archive>
//         void serialize(Archive & ar, const unsigned int version)
//         {
//             ar & termIdList_ ;
//
//         }
//         DATA_IO_LOAD_SAVE(TgLabelCache, &termIdList_)
//
// };
//
//
// template <typename T = TIL>
// class TgLabelInDoc
// {
//     public:
//         TgLabelInDoc()
//         {
//         }
//         TgLabelInDoc(TgLabelId labelId, const T& value):labelId_(labelId), value_(value)
//         {
//         }
//         TgLabelId labelId_;
//         T value_;
//         // 			loc_t position_;
//         friend class boost::serialization::access;
//         template<class Archive>
//         void serialize(Archive & ar, const unsigned int version)
//         {
//             ar & labelId_;
//             ar & value_;
//
//         }
//         DATA_IO_LOAD_SAVE(TgLabelInDoc, &labelId_&value_)
// };
//
//
// class TgWordPairKey
// {
//     public:
//         TgWordPairKey()
//         {
//         }
//         TgWordPairKey(uint32_t id1, uint32_t id2, uint8_t indicator):
//             id1_(id1), id2_(id2), indicator_(indicator)
//         {
//         }
//         uint32_t id1_;
//         uint32_t id2_;
//         uint8_t indicator_;
//
//         inline int compare(const TgWordPairKey& rightItem) const
//         {
//            if(this->id1_!=rightItem.id1_)
//                return((int)this->id1_-(int)rightItem.id1_);
//            if(this->id2_!=rightItem.id2_)
//                return((int)this->id2_-(int)rightItem.id2_);
//            return((int)this->indicator_-(int)rightItem.indicator_);
//         }
//
//         friend class boost::serialization::access;
//         template<class Archive>
//         void serialize(Archive & ar, const unsigned int version)
//         {
//             ar & id1_;
//             ar & id2_;
//             ar & indicator_;
//
//         }
//         DATA_IO_LOAD_SAVE(TgWordPairKey, &id1_&id2_&indicator_)
// };
//
//
// class CandidateTerm
// {
//     public:
//         termid_t oId_;
//         termid_t nId_;
//         std::string oStr_;
//         std::string nStr_;
//         char language_;
//         bool isConj_;
//         loc_t pos_;
//         char postag_;
// };
//
//
class TgLabelInfo
{
public:
    TgLabelInfo():min_query_distance_(10)
    {}
    TgLabelInfo(uint32_t docCount):doc_invert_(docCount), label_score_(1.0), min_query_distance_(10)
    {}
    ~TgLabelInfo() {}

    inline void set(uint32_t i)
    {
        doc_invert_.set(i);
    }
    uint32_t label_id_;
    std::vector<izenelib::util::UString> termList_;
    std::vector<termid_t> termIdList_;
    izenelib::util::UString name_;
    boost::dynamic_bitset<> doc_invert_;
    double label_score_;
    loc_t min_query_distance_;
    uint32_t originLabelId_;

};


}



#endif /* TGTYPES_H_ */

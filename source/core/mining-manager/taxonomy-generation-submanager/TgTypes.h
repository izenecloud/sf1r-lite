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
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <util/ustring/UString.h>
#include <boost/dynamic_bitset.hpp>
#include <common/type_defs.h>
#include <util/izene_serialization.h>
#include <idmlib/concept-clustering/cc_types.h>
#include <3rdparty/msgpack/msgpack.hpp>
namespace sf1r
{

#define TG_VAR_DEF(var,path) var(path+"/"+#var+".tg")


// typedef std::pair<izenelib::util::UString, std::vector<uint32_t> > ne_item_type;
// struct ne_result_type
// {
//     izenelib::util::UString type_;
//     std::vector<ne_item_type > itemList_;
//     friend class boost::serialization::access;
//     template<class Archive>
//     void serialize(Archive & ar, const unsigned int version)
//     {
//         ar & type_;
//         ar & itemList_;
//     }
//     DATA_IO_LOAD_SAVE(ne_result_type, &type_&itemList_);
// 
//     MSGPACK_DEFINE(type_,itemList_);
// };
// typedef std::vector<ne_result_type> ne_result_list_type;


struct NEItem
{
    izenelib::util::UString text;
    std::vector<wdocid_t> doc_list;
    
    MSGPACK_DEFINE(text, doc_list);
};

struct NEResult
{
    izenelib::util::UString type;
    std::vector<NEItem> item_list;
    
    MSGPACK_DEFINE(type, item_list);
};

typedef std::vector<NEResult> NEResultList;


typedef std::vector<termid_t> TIL;
typedef idmlib::cc::ClusterRep<wdocid_t> TgClusterRep;

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

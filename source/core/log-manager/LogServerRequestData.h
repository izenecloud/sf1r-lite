#ifndef LOG_SERVER_REQUEST_DATA_H_
#define LOG_SERVER_REQUEST_DATA_H_

#include <3rdparty/msgpack/msgpack.hpp>
#include <recommend-manager/common/RecTypes.h>

#include <string>
#include <vector>
#include <map>

namespace sf1r
{

inline std::ostream& operator<<(std::ostream& os, uint128_t uint128)
{
    os << hex << uint64_t(uint128>>64) << uint64_t(uint128) << dec;
    return os;
}

struct LogServerRequestData
{
};

struct InsertData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string key_;

    MSGPACK_DEFINE(service_, collection_, key_);
};

struct InsertWithValuesData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string key_;
    std::map<std::string, std::string> values_;

    MSGPACK_DEFINE(service_, collection_, key_, values_);
};

struct GetCurrentTopKData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    uint32_t limit_;

    MSGPACK_DEFINE(service_, collection_, limit_);
};

struct GetTopKData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string begin_time_;
    std::string end_time_;
    uint32_t limit_;

    MSGPACK_DEFINE(service_, collection_, begin_time_, end_time_, limit_);
};

struct GetCurrentDVCData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;

    MSGPACK_DEFINE(service_, collection_);
};

struct GetDVCData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string begin_time_;
    std::string end_time_;

    MSGPACK_DEFINE(service_, collection_, begin_time_, end_time_);
};

struct GetValueData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string time_;
    uint32_t limit_;

    MSGPACK_DEFINE(service_, collection_, time_, limit_);
};


struct GetValueAndCountData: public LogServerRequestData
{
    std::string service_;
    std::string collection_;
    std::string begin_time_;

    MSGPACK_DEFINE(service_, collection_, begin_time_);
};

struct GetAllCollectionData: public LogServerRequestData
{
    std::string service_;
    std::string begin_time_;

    MSGPACK_DEFINE(service_, begin_time_);
};

struct InsertPropLabelData: public LogServerRequestData
{
    std::string collection_;
    std::string label_name_;
    uint64_t hitnum_;

    MSGPACK_DEFINE(collection_, label_name_, hitnum_);
};

struct GetPropLabelData: public LogServerRequestData
{
    std::string collection_;

    MSGPACK_DEFINE(collection_);
};

struct DelPropLabelData: public LogServerRequestData
{
    std::string collection_;

    MSGPACK_DEFINE(collection_);
};

struct UUID2DocidList : public LogServerRequestData
{
    typedef std::vector<uint128_t> DocidListType;

    uint128_t uuid_;
    DocidListType docidList_;

    std::string toString() const;

    MSGPACK_DEFINE(uuid_, docidList_)
};

struct SynchronizeData : public LogServerRequestData
{
    // reserved
    std::string msg_;

    MSGPACK_DEFINE(msg_)
};

struct Docid2UUID : public LogServerRequestData
{
    uint128_t docid_;
    uint128_t uuid_;

    std::string toString() const;

    MSGPACK_DEFINE(docid_, uuid_)
};

struct CreateScdDocRequestData : public LogServerRequestData
{
    uint128_t docid_;
    std::string content_;
    std::string collection_;

    MSGPACK_DEFINE(docid_, content_, collection_)
};

struct DeleteScdDocRequestData : public LogServerRequestData
{
    uint128_t docid_;
    std::string collection_;

    MSGPACK_DEFINE(docid_, collection_)
};

struct GetScdFileRequestData : public LogServerRequestData
{
    std::string username_;
    std::string host_;
    std::string path_;
    std::string collection_;

    MSGPACK_DEFINE(username_, host_, path_, collection_)
};

struct GetScdFileResponseData
{
    bool success_;
    std::string error_;
    std::string scdFileName_;

    MSGPACK_DEFINE(success_, error_, scdFileName_)
};

struct StrIdToItemIdRequestData
{
    std::string collection_;
    std::string strId_;

    StrIdToItemIdRequestData() {}

    StrIdToItemIdRequestData(const std::string& collection, const std::string strId)
        : collection_(collection), strId_(strId)
    {}

    MSGPACK_DEFINE(collection_, strId_)
};

struct ItemIdToStrIdRequestData
{
    std::string collection_;
    itemid_t itemId_;

    ItemIdToStrIdRequestData() : itemId_(0) {}

    ItemIdToStrIdRequestData(const std::string& collection, itemid_t itemId)
        : collection_(collection), itemId_(itemId)
    {}

    MSGPACK_DEFINE(collection_, itemId_)
};

struct OldUUIDData : public LogServerRequestData
{
    bool success_;
    uint128_t docid_;
    std::string olduuid_;
    MSGPACK_DEFINE(success_, docid_, olduuid_)
};

struct OldDocIdData: public LogServerRequestData
{
    bool success_;
    uint128_t uuid_;
    std::string olddocid_;
    MSGPACK_DEFINE(success_, uuid_, olddocid_)
};

struct DelOldDocIdData: public LogServerRequestData
{
    typedef std::vector<uint128_t> UuidListType;

    bool success_;
    std::string olddocid_;
    UuidListType uuid_list_;
    //std::string toString() const;

    MSGPACK_DEFINE(success_, olddocid_, uuid_list_)
};

}

#endif /* LOG_SERVER_REQUEST_DATA_H_ */

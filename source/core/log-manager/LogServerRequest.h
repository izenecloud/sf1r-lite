#ifndef _LOG_SERVER_REQUEST_H_
#define _LOG_SERVER_REQUEST_H_

#include "LogServerRequestData.h"

namespace sf1r
{

class LogServerRequest
{
public:
    typedef std::string method_t;

    /// add method here
    enum METHOD
    {
        METHOD_TEST = 0,
        METHOD_UPDATE_UUID,
        METHOD_SYNCHRONIZE,
        METHOD_GET_UUID,
        METHOD_GET_DOCID_LIST,
        METHOD_CREATE_SCD_DOC,
        METHOD_DELETE_SCD_DOC,
        METHOD_GET_SCD_FILE,
        METHOD_STRID_TO_ITEMID,
        METHOD_ITEMID_TO_STRID,
        METHOD_GET_MAX_ITEMID,
        METHOD_ADD_OLD_UUID,     // when uuid change to a new one, log the old uuid for the changing doc
        METHOD_ADD_OLD_DOCID,    // when doc moved to a new uuid, add the docid to the old uuid group.
        METHOD_DEL_OLD_DOCID,    // when doc deleted, remove the docid from all the uuid groups who hold it.
        METHOD_GET_OLD_UUID,    
        METHOD_GET_OLD_DOCID,  
        COUNT_OF_METHODS
    };

    static const method_t method_names[COUNT_OF_METHODS];

    METHOD method_;

public:
    LogServerRequest(const METHOD& method) : method_(method) {}
    virtual ~LogServerRequest() {}
};

template <typename RequestDataT>
class LogRequestRequestT : public LogServerRequest
{
public:
    LogRequestRequestT(METHOD method)
        : LogServerRequest(method)
    {
    }

    RequestDataT param_;

    MSGPACK_DEFINE(param_)
};

class UpdateUUIDRequest : public LogRequestRequestT<UUID2DocidList>
{
public:
    UpdateUUIDRequest()
        : LogRequestRequestT<UUID2DocidList>(METHOD_UPDATE_UUID)
    {
    }
};

class SynchronizeRequest : public LogRequestRequestT<SynchronizeData>
{
public:
    SynchronizeRequest()
        : LogRequestRequestT<SynchronizeData>(METHOD_SYNCHRONIZE)
    {
    }
};

class GetUUIDRequest : public LogRequestRequestT<Docid2UUID>
{
public:
    GetUUIDRequest()
        : LogRequestRequestT<Docid2UUID>(METHOD_GET_UUID)
    {
    }
};

class GetDocidListRequest : public LogRequestRequestT<UUID2DocidList>
{
public:
    GetDocidListRequest()
        : LogRequestRequestT<UUID2DocidList>(METHOD_GET_DOCID_LIST)
    {
    }
};

class CreateScdDocRequest : public LogRequestRequestT<CreateScdDocRequestData>
{
public:
    CreateScdDocRequest()
        : LogRequestRequestT<CreateScdDocRequestData>(METHOD_CREATE_SCD_DOC)
    {
    }
};

class DeleteScdDocRequest : public LogRequestRequestT<DeleteScdDocRequestData>
{
public:
    DeleteScdDocRequest()
        : LogRequestRequestT<DeleteScdDocRequestData>(METHOD_DELETE_SCD_DOC)
    {
    }
};

class GetScdFileRequest : public LogRequestRequestT<GetScdFileRequestData>
{
public:
    GetScdFileRequest()
        : LogRequestRequestT<GetScdFileRequestData>(METHOD_GET_SCD_FILE)
    {
    }
};

class AddOldUUIDRequest : public LogRequestRequestT<OldUUIDData>
{
public:
    AddOldUUIDRequest ()
        : LogRequestRequestT<OldUUIDData>(METHOD_ADD_OLD_UUID)
    {
    }
};

class AddOldDocIdRequest : public LogRequestRequestT<OldDocIdData>
{
public:
    AddOldDocIdRequest ()
        : LogRequestRequestT<OldDocIdData>(METHOD_ADD_OLD_DOCID)
    {
    }
};

class DelOldDocIdRequest : public LogRequestRequestT<DelOldDocIdData>
{
public:
    DelOldDocIdRequest ()
        : LogRequestRequestT<DelOldDocIdData>(METHOD_DEL_OLD_DOCID)
    {
    }
};

class GetOldUUIDRequest : public LogRequestRequestT<OldUUIDData>
{
public:
    GetOldUUIDRequest ()
        : LogRequestRequestT<OldUUIDData>(METHOD_GET_OLD_UUID)
    {
    }
};

class GetOldDocIdRequest : public LogRequestRequestT<OldDocIdData>
{
public:
    GetOldDocIdRequest ()
        : LogRequestRequestT<OldDocIdData>(METHOD_GET_OLD_DOCID)
    {
    }
};


}

#endif /* _LOG_SERVER_REQUEST_H_ */

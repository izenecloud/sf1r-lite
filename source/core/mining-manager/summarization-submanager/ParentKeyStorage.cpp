#include "ParentKeyStorage.h"

#include <common/Utilities.h>
#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <iostream>

namespace
{
static const std::string p2c_path("/parent_to_children");
static const std::string c2p_path("/child_to_parent");
}

namespace sf1r
{

ParentKeyStorage::ParentKeyStorage(
        const std::string& db_dir,
        CommentCacheStorage* comment_cache_storage,
        unsigned bufferSize)
    : comment_cache_storage_(comment_cache_storage)
{
}

ParentKeyStorage::~ParentKeyStorage()
{
}

bool ParentKeyStorage::GetChildren(const ParentKeyType& parent, std::vector<ChildKeyType>& children)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetDocidListRequest docidListReq;
    UUID2DocidList docidListResp;

    docidListReq.param_.uuid_ = parent;

    conn.syncRequest(docidListReq, docidListResp);
    if (docidListReq.param_.uuid_ != docidListResp.uuid_)
        return false;

    children.swap(docidListResp.docidList_);
    return true;
}

bool ParentKeyStorage::GetParent(const ChildKeyType& child, ParentKeyType& parent)
{
    LogServerConnection& conn = LogServerConnection::instance();
    GetUUIDRequest uuidReq;
    Docid2UUID uuidResp;

    uuidReq.param_.docid_ = child;

    conn.syncRequest(uuidReq, uuidResp);
    if (uuidReq.param_.docid_ != uuidResp.docid_)
        return false;

    parent = uuidResp.uuid_;
    return true;
}

}

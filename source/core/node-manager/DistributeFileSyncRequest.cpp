#include "DistributeFileSyncRequest.h"

namespace sf1r
{

const FileSyncServerRequest::method_t FileSyncServerRequest::method_names[] =
{
    "test",
    "get_reqlog",
    "get_scdlist",
    "get_file",
    "ready_receive",
    "finish_receive",
    "report_status_req",
    "report_status_rsp"
};

}


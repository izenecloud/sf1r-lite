#include "LogServerRequest.h"

namespace sf1r
{

const LogServerRequest::method_t LogServerRequest::method_names[] =
        {
            "update_uuid",
            "synchronize",
            "get_uuid",
            "get_docid_list",
            "create_scd_doc",
            "delete_scd_doc",
            "get_scd_file"
        };

}

#include "LogServerRequest.h"

namespace sf1r
{

const LogServerRequest::method_t LogServerRequest::method_names[] =
        {
            "test",
            "update_uuid",
            "synchronize",
            "get_uuid",
            "get_docid_list",
            "create_scd_doc",
            "delete_scd_doc",
            "get_scd_file",
            "strid_to_itemid",
            "itemid_to_strid",
            "get_max_itemid",
            "add_old_uuid",
            "add_old_docid",
            "del_old_docid",
            "get_old_uuid",
            "get_old_docid",
        };

}

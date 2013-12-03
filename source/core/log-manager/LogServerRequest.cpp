#include "LogServerRequest.h"

namespace sf1r
{

const LogServerRequest::method_t LogServerRequest::method_names[] =
        {
            "test",
       
            "insert",
            "insert_with_values",
            "get_current_topk",
            "get_topk",
            "get_current_dvc",
            "get_dvc",
            "get_values",
            "get_value_and_count",
            "get_all_collection",
            "insert_prop_label",
            "get_prop_label",
            "del_prop_label"

            "strid_to_itemid",
            "itemid_to_strid",
            "get_max_itemid",

            "update_uuid",
            "synchronize",
            "get_uuid",
            "get_docid_list",
            "create_scd_doc",
            "delete_scd_doc",
            "get_scd_file",
            "add_old_uuid",
            "add_old_docid",
            "del_old_docid",
            "get_old_uuid",
            "get_old_docid"
        };

}

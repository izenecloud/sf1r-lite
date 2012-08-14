#include "ImageServerRequest.h"

namespace sf1r
{

const ImageServerRequest::method_t ImageServerRequest::method_names[] =
        {
            "test",
            "get_image_color",
            "compute_image_color",
            "upload_image",
            "delete_image",
            "export_image",
        };

}

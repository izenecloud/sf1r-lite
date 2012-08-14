#ifndef _IMAGE_SERVER_REQUEST_H_
#define _IMAGE_SERVER_REQUEST_H_

#include "ImageServerRequestData.h"
#include <sf1r-net/RpcServerRequest.h>

namespace sf1r
{

class ImageServerRequest : public RpcServerRequest
{
public:
    typedef std::string method_t;

    /// add method here
    enum METHOD
    {
        METHOD_TEST = 0,
        METHOD_GET_IMAGE_COLOR,
        METHOD_COMPUTE_IMAGE_COLOR,
        METHOD_UPLOAD_IMAGE,
        METHOD_DELETE_IMAGE,
        METHOD_EXPORT_IMAGE,
        COUNT_OF_METHODS
    };

    static const method_t method_names[COUNT_OF_METHODS];

public:
    ImageServerRequest(const int& method) : RpcServerRequest(method) {}
};

class GetImageColorRequest : public RpcRequestRequestT<ImageColorData, ImageServerRequest>
{
public:
    GetImageColorRequest()
        : RpcRequestRequestT<ImageColorData, ImageServerRequest>(METHOD_GET_IMAGE_COLOR)
    {
    }
};

class ComputeImageColorRequest : public RpcRequestRequestT<ComputeImageColorData, ImageServerRequest>
{
public:
    ComputeImageColorRequest()
        : RpcRequestRequestT<ComputeImageColorData, ImageServerRequest>(METHOD_COMPUTE_IMAGE_COLOR)
    {
    }
};

class UploadImageRequest : public RpcRequestRequestT<UploadImageData, ImageServerRequest>
{
public:
    UploadImageRequest()
        : RpcRequestRequestT<UploadImageData, ImageServerRequest>(METHOD_UPLOAD_IMAGE)
    {
    }
};

class DeleteImageRequest : public RpcRequestRequestT<DeleteImageData, ImageServerRequest>
{
public:
    DeleteImageRequest()
        : RpcRequestRequestT<DeleteImageData, ImageServerRequest>(METHOD_DELETE_IMAGE)
    {
    }
};

class ExportImageRequest : public RpcRequestRequestT<ExportImageData, ImageServerRequest>
{
public:
    ExportImageRequest()
        : RpcRequestRequestT<ExportImageData, ImageServerRequest>(METHOD_EXPORT_IMAGE)
    {
    }
};


}

#endif /* _IMAGE_SERVER_REQUEST_H_ */

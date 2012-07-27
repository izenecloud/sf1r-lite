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


}

#endif /* _IMAGE_SERVER_REQUEST_H_ */

#ifndef IMAGE_SERVER_REQUEST_DATA_H_
#define IMAGE_SERVER_REQUEST_DATA_H_

#include <sf1r-net/RpcServerRequestData.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <string>
#include <vector>

namespace sf1r
{

struct ImageColorData : public RpcServerRequestData
{
    bool success;
    std::string img_file;
    std::string result_img_color;
    MSGPACK_DEFINE(success, img_file, result_img_color)
};

}

#endif /* IMAGE_SERVER_REQUEST_DATA_H_ */

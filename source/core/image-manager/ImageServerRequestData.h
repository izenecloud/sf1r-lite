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

struct ComputeImageColorData : public RpcServerRequestData
{
    bool success;
    std::string filepath;
    int  filetype;
    MSGPACK_DEFINE(success, filepath, filetype)
};

struct UploadImageData : public RpcServerRequestData
{
    bool success;
    int  param_type;  // indicate the img_file param to stand for the file path or the real image data.
    std::string img_file;
    MSGPACK_DEFINE(success, param_type, img_file)
};

struct DeleteImageData : public RpcServerRequestData
{
    bool success;
    std::string img_file;
    MSGPACK_DEFINE(success, img_file)
};

struct ExportImageData : public RpcServerRequestData
{
    std::string img_log_file;   // the file which will contain the uploaded file list.
    std::string out_dir;
    MSGPACK_DEFINE(img_log_file, out_dir)
};

}

#endif /* IMAGE_SERVER_REQUEST_DATA_H_ */

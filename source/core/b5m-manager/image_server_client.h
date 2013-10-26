#ifndef SF1R_B5MMANAGER_IMAGE_SERVERCLIENT_H_
#define SF1R_B5MMANAGER_IMAGE_SERVERCLIENT_H_

#include "b5m_types.h"
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <sf1r-net/RpcServerConnectionConfig.h>
#include <sf1r-net/RpcServerConnection.h>

NS_SF1R_B5M_BEGIN
class ImageServerClient {
public:
    static bool Init(const sf1r::RpcServerConnectionConfig& config);
    static bool Test();
    static bool GetImageColor(const std::string& img_file, std::string& ret_color);
    static bool UploadImageFile(const std::string& img_file, std::string& ret_file_name);
    static bool UploadImageData(const std::string& img_data, std::string& ret_file_name);
};

NS_SF1R_B5M_END

#endif


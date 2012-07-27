#ifndef SF1R_B5MMANAGER_IMAGE_SERVERCLIENT_H_
#define SF1R_B5MMANAGER_IMAGE_SERVERCLIENT_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <sf1r-net/RpcServerConnectionConfig.h>
#include <sf1r-net/RpcServerConnection.h>
#include "b5m_types.h"

namespace sf1r {
    class ImageServerClient {
    public:
        static bool Init(const RpcServerConnectionConfig& config);
        static bool Test();
        static bool GetImageColor(const std::string& img_file, string& ret_color);
    };

}

#endif


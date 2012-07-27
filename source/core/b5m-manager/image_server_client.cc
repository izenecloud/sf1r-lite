#include "image_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/Utilities.h>
#include <image-manager/ImageServerRequest.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

bool ImageServerClient::Init(const RpcServerConnectionConfig& config)
{
    RpcServerConnection& conn = RpcServerConnection::instance();
    if(!conn.init(config))
    {
        return false;
    }
    return true;
}

bool ImageServerClient::Test()
{
    RpcServerConnection& conn = RpcServerConnection::instance();
    if(!conn.testServer())
    {
        std::cout << "Image server test failed!" << endl;
        return false;
    }
    return true;
}

bool ImageServerClient::GetImageColor(const std::string& img_file, string& ret_color)
{
    RpcServerConnection& conn = RpcServerConnection::instance();
    
    GetImageColorRequest req;    
    req.param_.img_file = img_file;
    ImageColorData rsp;
    conn.syncRequest(req, rsp);
    ret_color = rsp.result_img_color;
    return rsp.success;
}


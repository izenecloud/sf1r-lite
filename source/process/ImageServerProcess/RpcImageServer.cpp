#include "RpcImageServer.h"
#include "ImageServerCfg.h"
#include "ImageProcess.h"
#include <image-manager/ImageServerStorage.h>
#include <image-manager/ImageServerRequest.h>
#include "errno.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

RpcImageServer::RpcImageServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
{
}

RpcImageServer::~RpcImageServer()
{
    std::cout << "~RpcImageServer()" << std::endl;
    stop();
}

void RpcImageServer::start()
{
    instance.listen(host_, port_);
    instance.start(threadNum_);
}

void RpcImageServer::join()
{
    instance.join();
}

void RpcImageServer::run()
{
    start();
    join();
}

void RpcImageServer::stop()
{
    instance.end();
    instance.join();
}

void RpcImageServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);

        if (method == ImageServerRequest::method_names[ImageServerRequest::METHOD_TEST])
        {
            msgpack::type::tuple<bool> params;
            req.params().convert(&params);

            req.result(true);
        }
        else if (method == ImageServerRequest::method_names[ImageServerRequest::METHOD_GET_IMAGE_COLOR])
        {
            msgpack::type::tuple<ImageColorData> params;
            req.params().convert(&params);
            ImageColorData& reqdata = params.get<0>();

            reqdata.success = ImageServerStorage::get()->GetImageColor(reqdata.img_file, reqdata.result_img_color);
            if(!reqdata.success)
            {
                // try to recalculate the color
                std::string picFilePath = ImageServerCfg::get()->getImageFilePath() + "/" + reqdata.img_file;
                int indexCollection[COLOR_RET_NUM];
                bool ret = getImageColor(picFilePath.c_str(), indexCollection, COLOR_RET_NUM);
                if(ret){
                    std::ostringstream oss;
                    for(std::size_t i = 0; i < COLOR_RET_NUM - 1; ++i)
                    {
                        oss << indexCollection[i] << ",";
                    }
                    oss << indexCollection[COLOR_RET_NUM - 1]; 

                    reqdata.success = ImageServerStorage::get()->SetImageColor(reqdata.img_file, oss.str());
                    reqdata.result_img_color = oss.str();
                    //LOG(INFO) << "got request image color for fileName=" << reqdata.img_file << "result:" <<
                    //   oss.str() << std::endl;
                }else{
                    LOG(WARNING) << "getImageColor failed imagePath=" << reqdata.img_file << std::endl;
                    reqdata.success = false;
                }
            }
            req.result(reqdata);
        }
        else
        {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    }
    catch (const msgpack::type_error& e)
    {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
    }
    catch (const std::exception& e)
    {
        req.error(std::string(e.what()));
    }
}

}

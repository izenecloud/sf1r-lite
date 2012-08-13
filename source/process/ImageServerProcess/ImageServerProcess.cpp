#include "ImageServerProcess.h"
#include "ImageServerCfg.h"
#include "RpcImageServer.h"
#include "ImageProcessTask.h"
#include <image-manager/ImageServerStorage.h>

#include <common/OnSignal.h>
#include <glog/logging.h>
#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>
#include <iostream>

namespace sf1r
{

#define RETURN_ON_FAILURE(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    return false;                                                           \
}

#define LOG_SERVER_ASSERT(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    std::cerr << "Assertion failed: " #condition                            \
              << std::endl;                                                 \
    return false;                                                           \
}


ImageServerProcess::ImageServerProcess()
{
}

ImageServerProcess::~ImageServerProcess()
{
}

bool ImageServerProcess::init(const std::string& cfgFile)
{
    // Parse config first
    RETURN_ON_FAILURE(ImageServerCfg::get()->parse(cfgFile));

    RETURN_ON_FAILURE(ImageServerStorage::get()->init(ImageServerCfg::get()->getImageColorDB(),
            ImageServerCfg::get()->getImageFileServer()));
    RETURN_ON_FAILURE(initRpcImageServer());

    addExitHook(boost::bind(&ImageServerProcess::stop, this));

    return true;
}

void ImageServerProcess::start()
{
    LOG(INFO) << "\tRPC Server    : port=" << rpcImageServer_->getPort();
    rpcImageServer_->start();
}

void ImageServerProcess::join()
{
    if(rpcImageServer_)
        rpcImageServer_->join();
}

void ImageServerProcess::stop()
{
    ImageProcessTask::instance()->stop();
    std::cout << "stop ImageServerProcess" << std::endl;
    if(rpcImageServer_)
        rpcImageServer_->stop();
    ImageServerStorage::get()->close();
    rpcImageServer_.reset();
}

bool ImageServerProcess::initRpcImageServer()
{
    rpcImageServer_.reset(
        new RpcImageServer(
            ImageServerCfg::get()->getLocalHost(),
            ImageServerCfg::get()->getRpcServerPort(),
            ImageServerCfg::get()->getRpcThreadNum()
        ));

    if (rpcImageServer_)
    {
        return true;
    }

    return false;
}

}

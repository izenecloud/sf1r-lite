#include "RpcImageServer.h"
#include "ImageServerCfg.h"
#include "ImageProcess.h"
#include "ImageProcessTask.h"
#include <image-manager/ImageServerStorage.h>
#include <image-manager/ImageServerRequest.h>
#include "errno.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>

#include <tfs_client_api.h>

using namespace tfs::client;
using namespace tfs::common;

namespace sf1r
{

RpcImageServer::RpcImageServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
    , is_exporting_(false)
    , export_thread_(NULL)
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
    if(export_thread_)
    {
        export_thread_->interrupt();
        export_thread_->join();
        delete export_thread_;
    }
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
                //std::string picFilePath = ImageServerCfg::get()->getImageFilePath() + "/" + reqdata.img_file;
                char* data = NULL;
                std::size_t datasize = 0;
                if(ImageServerStorage::get()->DownloadImageData(reqdata.img_file, data, datasize))
                {
                    int indexCollection[COLOR_RET_NUM];
                    bool ret = getImageColor(data, datasize, indexCollection, COLOR_RET_NUM);
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
                    delete[] data;
                }
                else
                {
                    LOG(INFO) << "getImage data failed while get image color imagePath=" << reqdata.img_file << std::endl;
                    reqdata.success = false;
                }
            }
            req.result(reqdata);
        }
        else if (method == ImageServerRequest::method_names[ImageServerRequest::METHOD_UPLOAD_IMAGE])
        {
            msgpack::type::tuple<UploadImageData> params;
            req.params().convert(&params);
            UploadImageData& reqdata = params.get<0>();
            std::string ret_file;
            if(reqdata.param_type == 0)
            {
                reqdata.success = ImageServerStorage::get()->UploadImageFile(reqdata.img_file, ret_file);
            }
            else if(reqdata.param_type == 1)
            {
                reqdata.success = ImageServerStorage::get()->UploadImageData(reqdata.img_file.data(),
                    reqdata.img_file.size(), ret_file);
            }
            else
            {
                reqdata.success = false;
            }
            if(reqdata.success)
            {
                // write log file
                //
                ofstream ofs;
                try
                {
                    ofs.open(ImageServerCfg::get()->getUploadImageLog().c_str(), ofstream::app);
                    ofs << ret_file.c_str() << std::endl;
                    ofs.close();
                }
                catch(std::exception& e)
                {
                    LOG(ERROR) << "write upload log failed: " << ret_file << std::endl;
                    ofs.close();
                }
            }
            else
            {
                LOG(WARNING) << "file upload failed: " << reqdata.img_file << std::endl;
            }
            reqdata.img_file = ret_file;
            req.result(reqdata);
        }
        else if (method == ImageServerRequest::method_names[ImageServerRequest::METHOD_DELETE_IMAGE])
        {
            msgpack::type::tuple<DeleteImageData> params;
            req.params().convert(&params);
            DeleteImageData& reqdata = params.get<0>();
            reqdata.success = ImageServerStorage::get()->DeleteImage(reqdata.img_file);
            req.result(reqdata);
        }
        else if (method == ImageServerRequest::method_names[ImageServerRequest::METHOD_EXPORT_IMAGE])
        {
            msgpack::type::tuple<ExportImageData> params;
            req.params().convert(&params);
            ExportImageData& reqdata = params.get<0>();
            req.result(reqdata);
            if(is_exporting_)
            {
                LOG(WARNING) << "image is already exporting! Ignore any more request." << std::endl;
                return;
            }
            is_exporting_ = true;
            if(export_thread_)
            {
                delete export_thread_;
            }

            export_thread_ = new boost::thread(&RpcImageServer::exportimage, this, 
                    reqdata.img_log_file, reqdata.out_dir);

        }
        else if(method == ImageServerRequest::method_names[ImageServerRequest::METHOD_COMPUTE_IMAGE_COLOR])
        {
            msgpack::type::tuple<ComputeImageColorData> params;
            req.params().convert(&params);
            ComputeImageColorData& reqdata = params.get<0>();

            Request *image_computereq = new Request;
            image_computereq->filePath = reqdata.filepath;
            image_computereq->filetype = reqdata.filetype;
            LOG(INFO) << "got image file color computing request:" << reqdata.filepath << endl;
            MSG msg(image_computereq);
            reqdata.success = true;
            if(!ImageProcessTask::instance()->put(msg)){ 
                LOG(ERROR) << "failed to enqueue fileName:" << reqdata.filepath << endl;
                reqdata.success = false;
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

void RpcImageServer::exportimage(const std::string& log_file, const std::string& outdir)
{
    ifstream ifs;
    try
    {
    ifs.open(log_file.c_str(), ifstream::in);

    while(ifs.good())
    {
        char file_name[TFS_FILE_LEN];
        ifs.getline(file_name, TFS_FILE_LEN);
        char* buffer = NULL;
        std::size_t buf_size = 0;
        bool ret = ImageServerStorage::get()->DownloadImageData(file_name, buffer, buf_size);
        if(ret)
        {
            ofstream ofs;
            try
            {
                ofs.open((outdir + "/" + file_name).c_str(), ofstream::trunc);
                ofs.write(buffer, buf_size);
                ofs.close();
            }
            catch(std::exception& e)
            {
                ofs.close();
                LOG(WARNING) << "export failed (save data to out file error) : " << e.what() << ",file: " << file_name << std::endl;
            }
            delete[] buffer;
        }
        else
        {
            LOG(WARNING) << "export failed (download error): " << file_name << std::endl;
        }
        boost::this_thread::interruption_point();
    }

    ifs.close();
    }
    catch(boost::thread_interrupted& )
    {
        ifs.close();
    }
    catch(std::exception& e)
    {
        ifs.close();
        LOG(WARNING) << "exception while exporting: " << e.what() << std::endl;
    }
    is_exporting_ = false;
}

}

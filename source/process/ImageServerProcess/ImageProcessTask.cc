#include "ImageProcessTask.h"
#include "ImageProcess.h"
#include "TimeEstimate.h"
#include "Request.h"
#include <image-manager/ImageServerStorage.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <glog/logging.h>
#include <boost/bind.hpp>

#define IO_THREAD_NUM  1

using namespace sf1r;

ImageProcessTask::~ImageProcessTask()
{
}

bool ImageProcessTask::init(const std::string& imgfiledir, ssize_t threadCount)
{
    img_file_dir_ = imgfiledir;
    _stop=true;

    compute_threadIds_.resize(threadCount);
    return true;
}

bool ImageProcessTask::start()
{
    _stop=false;
    int ret=pthread_create(&io_threadId_,NULL,&ImageProcessTask::process,(void*)this);
    if(ret != 0){
        LOG(ERROR) << "[ImageProcessTask::start] failed to create new thread" << std::endl;
        return false;
    }

    ret=pthread_create(&db_write_threadId_,NULL,&ImageProcessTask::write_result,(void*)this);
    if(ret != 0){
        LOG(ERROR) << "[ImageProcessTask::start] failed to create new thread" << std::endl;
        return false;
    }

    for(size_t i=0;i<compute_threadIds_.size();++i){
        int ret=pthread_create(&compute_threadIds_[i],NULL,&ImageProcessTask::compute,(void*)this);
        if(ret != 0){
            LOG(ERROR) << "[ImageProcessTask::start] failed to create new thread" << std::endl;
            return false;
        }
    }

    return true;
}

bool ImageProcessTask::stop()
{
    std::cout << "stopping ImageProcessTask " << std::endl;
    _stop=true;
    pthread_join(io_threadId_,NULL);
    pthread_join(db_write_threadId_,NULL);

    for(size_t i=0;i<compute_threadIds_.size();++i){
        pthread_join(compute_threadIds_[i],NULL);
    }
    return _stop;
}

bool ImageProcessTask::put(MSG msg)
{
    if(_stop){
        return false;
    }else{
        msg_queue_.push(msg);
        return true;
    }
}

int ImageProcessTask::doJobs(const MSG& msg)
{
    std::string filePathRead(msg->filePath);
    std::ifstream ifs(filePathRead.c_str(),std::ios_base::in);
    std::string line;
    std::string docidTag("<DOCID>");
    std::string pictureTag("<Picture>");
    std::string colorTag("<Color>");
    int num = 0;
    while(ifs.good()){
        getline(ifs,line);
        line.erase(0,line.find_first_not_of(" "));
        line.erase(line.find_last_not_of(" ")+1);
        if(line.size()>0){
            if((line.compare(0,pictureTag.size(),pictureTag))==0){
                std::string picFilePath;
                if(line.at(0)=='/'){
                    picFilePath=std::string(line.c_str()+pictureTag.size()+1);
                }else{
                    picFilePath=std::string(line.c_str()+pictureTag.size());
                }
                picFilePath.erase(0,picFilePath.find_first_not_of(" "));
                picFilePath.erase(picFilePath.find_last_not_of(" ")+1);
                if(_stop)
                    return 0;

                std::string ret;
                if(ImageServerStorage::get()->GetImageColor(picFilePath, ret) && !ret.empty())
                {
                    if(++num%10000 == 0)
                    {
                        LOG(INFO) << "doJobs find exist image color in db: " << num << std::endl;
                    }
                    continue;
                }
                compute_queue_.push(picFilePath);
            }
        }
    }
    ifs.close();
    return 0;
}
int ImageProcessTask::doCompute(const std::string& img_file)
{
    static int num = 0;
    int indexCollection[COLOR_RET_NUM];
    std::string full_img_path;
    bool ret;
    //RECORD_TIME_START(ONEJOB);
    if(img_file_dir_.empty())
    {
        // using tfs file
        full_img_path = img_file;
        char* imgdata = NULL;
        std::size_t imgsize = 0;
        ret = ImageServerStorage::get()->DownloadImageData(full_img_path, imgdata, imgsize);
        if(ret)
        {
            ret = getImageColor(imgdata, imgsize, indexCollection, COLOR_RET_NUM);
        }
        else
        {
            LOG(INFO) << "Download Image data failed imagePath=" << img_file << std::endl;
        }
    }
    else
    {
        // use local image file
        full_img_path = std::string(img_file_dir_) + "/" + img_file;
        ret=getImageColor(full_img_path.c_str(),indexCollection,COLOR_RET_NUM);
    }
    //RECORD_TIME_END(ONEJOB);
    //CALC_TIME_ELAPSED(ONEJOB);
    if(ret){
        std::ostringstream oss;
        for(std::size_t i = 0; i < COLOR_RET_NUM - 1; ++i)
        {
            oss << indexCollection[i] << ",";
        }
        oss << indexCollection[COLOR_RET_NUM - 1];

        if(_stop)
            return 0;

        db_result_queue_.push(std::make_pair(img_file, oss.str()));

        if(++num%1000 == 0)
        {
            LOG(INFO) << "doCompute processed : " << num << std::endl;
        }
        //LOG(INFO) << "[ImageProcessTask::doJobs TIME:" <<
        //   __elapsed_ONEJOB <<  "fileName=" << picFilePath << "result:" <<
        //   oss.str() << "[ " << getColorText(indexCollection[0]) <<
        //   getColorText(indexCollection[1]) << getColorText(indexCollection[2]) <<
        //   "]" << std::endl;
    }else{
        //LOG(WARNING) << "getImageColor failed imagePath=" << full_img_path << std::endl;
    }
    return 0;
}

int ImageProcessTask::doWriteResultToDB(const std::pair<std::string, std::string>& result)
{
    static int new_num = 0;
    static int bak_num = 0;
    bool ret = ImageServerStorage::get()->SetImageColor(result.first, result.second);
    if(!ret)
    {
        LOG(ERROR) << "write image color to db failed imagePath=" << result.first << std::endl;
    }
    if( ++new_num > 30000 )
    {
        LOG(INFO) << "doWriteResultToDB is backingup." << std::endl;
        std::string bak_char;
        bak_char = "a";
        bak_char[0] = char('a' + (++bak_num%3));
        ImageServerStorage::get()->backup_imagecolor_db(".tmpbak-" + bak_char);
        new_num = 0;
    }
    return 0;
}

void *ImageProcessTask::write_result(void *arg)
{
    ImageProcessTask *task=(ImageProcessTask*)arg;
    bool flag;
    std::pair<std::string, std::string> img_color_result;
    while(!task->_stop){
        flag=false;
        while(!task->db_result_queue_.popNonBlocking(img_color_result)){
            if(task->_stop){
                flag=true;
                break;
            }
            usleep(100000);
        }
        if(!flag){
            task->doWriteResultToDB(img_color_result);
        }
    }
    while(task->db_result_queue_.popNonBlocking(img_color_result)){
    }
    return 0;
}


void *ImageProcessTask::compute(void *arg)
{
    ImageProcessTask *task=(ImageProcessTask*)arg;
    bool flag;
    std::string img_file;
    while(!task->_stop){
        flag=false;
        while(!task->compute_queue_.popNonBlocking(img_file)){
            if(task->_stop){
                flag=true;
                break;
            }
            usleep(100000);
        }
        if(!flag){
            task->doCompute(img_file);
        }
    }
    while(task->compute_queue_.popNonBlocking(img_file)){
    }
    return 0;
}

void *ImageProcessTask::process(void *arg)
{
    ImageProcessTask *task=(ImageProcessTask*)arg;
    MSG msg;
    bool flag;
    while(!task->_stop){
        flag=false;
        while(!task->msg_queue_.popNonBlocking(msg)){
            if(task->_stop){
                flag=true;
                break;
            }
            usleep(100000);
        }
        if(!flag){
            task->doJobs(msg);
        }
    }
    while(task->msg_queue_.popNonBlocking(msg)){
    }
    return 0;
}

#include "MessageQueue.h"
#include "Request.h"
#include "TimeEstimate.h"
#include "ImageProcess.h"
#include "ImageProcessTask.h"
#include <glog/logging.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <common/OnSignal.h>
#include <string>
#include "../common/ProcessOptions.h"
#include "ImageServerProcess.h"
#include "ImageServerCfg.h"

using namespace std;
using namespace sf1r;

enum SCD_TYPE{
    NOT_SCD=0,
    INSERT_SCD,
    UPDATE_SCD,
    DELETE_SCD
};

static void readFiles(const char *dirpath);
static SCD_TYPE checkSCDType(const char *filename);
static bool checkSCDFormat(const char *filename);

static __pid_t getPid()
{
    return ::getpid();
}

int main(int argc, char **argv)
{
    setupDefaultSignalHandlers();
    bool caughtException = false;
    try
    {
        ProcessOptions po;
        std::vector<std::string> args(argv + 1, argv + argc);

        if (!po.setImageServerProcessArgs(argv[0], args))
        {
            std::cout << "set arg failed." << endl;
            return -1;
        }
        __pid_t pid = getPid();
        LOG(INFO) << "\tImage Server Process : pid=" << pid;

        ImageServerProcess imageServerProcess;

        if (imageServerProcess.init(po.getConfigFile()))
        {
            initColorTable();
            initDistTable();
            LOG(INFO) << "initDistTable end" << endl;
            imageServerProcess.start();

            if(!ImageProcessTask::instance()->init(
                    ImageServerCfg::get()->getImageFilePath(),
                    ImageServerCfg::get()->getComputeThread())){
                LOG(ERROR) << "ImageProcessTask init failed" << endl;
                exit(-1);
            }
            if(!ImageProcessTask::instance()->start()){
                LOG(ERROR) << "ImageProcessTask start failed" << endl;
                exit(-1);
            }
            readFiles(ImageServerCfg::get()->getSCDFilePath().c_str());

            imageServerProcess.join();
        }
        else
        {
            imageServerProcess.stop();
            LOG(ERROR) << "Image Server start failed!" << endl;
            return -1;
        }
    }
    catch (const std::exception& e)
    {
        caughtException = true;
        std::cerr << e.what() << std::endl;
    }

    return caughtException?-1:0;
}

void readFiles(const char *dirPath)
{
    char dirpath[1024];
    strcpy(dirpath,dirPath);
    if(dirpath[strlen(dirpath)-1]=='/'){
    }else{
        strcat(dirpath,"/");
    }
    DIR *d;
    struct dirent *file;
    struct stat sb;
    if(!(d=opendir(dirpath))){
        LOG(ERROR) << "opendir failed,path=" << dirpath << endl;
        return;
    }
    char filepath[1024];
    while((file=readdir(d))!=NULL){
        if(strncmp(file->d_name,".",1)==0){
            continue;
        }
        strcpy(filepath,dirpath);
        strcat(filepath,file->d_name);
        if(stat(filepath,&sb)>=0){
            if(!S_ISDIR(sb.st_mode)){
                if(strlen(filepath)>4 && strncasecmp(filepath+strlen(filepath)-4,".scd",4)==0){
                    if(!checkSCDFormat(file->d_name)){
                        LOG(ERROR) << "Invalid SCD fileName : " << file->d_name << endl;
                        continue;
                    }
                    SCD_TYPE scd_type=checkSCDType(file->d_name);
                    if(scd_type==INSERT_SCD||scd_type==UPDATE_SCD){
                        Request *req=new Request;
                        req->filePath=std::string(filepath);
                        LOG(INFO) << "scd file is processing:" << filepath << endl;
                        MSG msg(req);
                        if(!ImageProcessTask::instance()->put(msg)){ 
                            LOG(ERROR) << "failed to enqueue fileName:" << filepath << endl;
                        }
                    }else{
                        LOG(INFO) << "file is not valid scd file:" << file->d_name << endl;
                    }
                }
            }
        }else{
            perror("stat error ");
        }
    }
    closedir(d);
}

SCD_TYPE checkSCDType(const char *filename)
{
    SCD_TYPE type=NOT_SCD;
    const static unsigned kTypeOffset=24;
    if(strlen(filename)>kTypeOffset){
        switch(tolower(filename[kTypeOffset])){
            case 'i':
                type=INSERT_SCD;
                break;
            case 'd':
                type=DELETE_SCD;
                break;
            case 'u':
                type=UPDATE_SCD;
                break;
            default:
                type=NOT_SCD;
                break;
        }
    }
    return type;
}

bool checkSCDFormat(const char *filename)
{
    //B-<document collector ID(2Bytes)>-<YYYYMMDDHHmm(12Bytes)>-
    //    <SSsss(second, millisecond, 5Bytes)>-<I/U/D/R(1Byte)>-
    //    <C/F(1Byte)>.SCD

    std::string fileName = std::string(filename);
    fileName=fileName.substr(0,fileName.length()-4);

    size_t pos = 0;
    std::string strVal;
    int val;

    if( fileName.length() != 27 )
        return false;
    if( fileName[pos] != 'B' && fileName[0] != 'b' )
        return false;
    if( fileName[++pos] != '-' ) 
        return false;

    pos++;
    strVal = fileName.substr( pos, 2 );        //collectorID
    sscanf( strVal.c_str(), "%d", &val );

    pos += 3;
    strVal = fileName.substr( pos, 4 );        //year
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 1970 )
        return false;

    pos += 4;
    strVal = fileName.substr( pos, 2 );        //month
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 1 || val > 12 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //day
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 1 || val > 31 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //hour
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 0 || val > 24 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //minute
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 0 || val >= 60 )
        return false;

    pos += 3;
    strVal = fileName.substr( pos, 2 );        //second
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 0 || val >= 60 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 3 );        //millisecond
    sscanf( strVal.c_str(), "%d", &val );
    if( val < 0 || val > 999 )
        return false;

    pos += 4;
    strVal = fileName.substr( pos, 1 );        //c
    char c = tolower( strVal[0] );
    if( c != 'i' && c != 'd' && c != 'u' && c != 'r' )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 1 );        //d
    c = tolower( strVal[0] );
    if( c != 'c' && c != 'f' )
        return false;

    
    return true;
}

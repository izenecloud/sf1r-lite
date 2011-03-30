/**
 * @file    LabelSynchronizer.h
 * @author  Peisheng Wang
 * @date    2009-12-14
 * @details
 *      It is a wrapper of rsync.
 */

#ifndef LABELSYNCHRONIZER_H_
#define LABELSYNCHRONIZER_H_

#include <util/izene_serialization.h>
#include <boost/filesystem.hpp>

namespace sf1r
{

struct RsyncInfo
{
    string user;
    string hostIP;
    string collectionName;
    string srcFilePath;
    string destFilePath;
    unsigned int srcFileSize;

    void setUser(const string& su)
    {
        user = su;
    }

    void setHostIp(const string& ip)
    {
        hostIP = ip;
    }

    void setDestPath(const string& path)
    {
        destFilePath = path+"/"+collectionName+".label";
    }

    string toRsyncString() const
    {
#ifdef USE_MF_LIGHT
        std::string cmd = "cp ";
        cmd += srcFilePath;
        cmd += " ";
        cmd += destFilePath;
#else
        std::string cmd = "rsync ";
        cmd += user;
        cmd += "@";
        cmd += hostIP;
        cmd += ":";
        cmd += srcFilePath;
        cmd += " ";
        cmd += destFilePath;
#endif
        return cmd;
    }

    DATA_IO_LOAD_SAVE(RsyncInfo, &user&collectionName&srcFilePath&srcFileSize);

};

class LabelSynchronizer
{

public:
    static bool rsyncLabel(const RsyncInfo& rsyncInfo, bool& result)
    {
        result = false;
        unsigned int destFileSize;

        if (boost::filesystem::exists(rsyncInfo.destFilePath) )
        {
            destFileSize = boost::filesystem::file_size(rsyncInfo.destFilePath);
            if (destFileSize == rsyncInfo.srcFileSize)
            {
                result = true;
                return false;
            }
        }
        std::string cmd = rsyncInfo.toRsyncString();
        std::cout<<"command: "<<cmd<<std::endl;
        std::system(cmd.c_str());

        destFileSize = boost::filesystem::file_size(rsyncInfo.destFilePath);
        if (destFileSize == rsyncInfo.srcFileSize)
        {
            result = true;
            return true;
        }
        else
        {
            result = false;
            return false;
        }
    }
};

}

MAKE_FEBIRD_SERIALIZATION(sf1r::RsyncInfo);

#endif /*LABELSYNCHRONIZER_H_*/

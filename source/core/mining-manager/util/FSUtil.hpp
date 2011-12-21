///
/// @file FSUtil.hpp
/// @brief Utility for file system operation
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-12-14
/// @date
///

#ifndef FSUTIL_HPP_
#define FSUTIL_HPP_

#include <mining-manager/MiningException.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <util/filesystem.h>

namespace sf1r
{

class FSUtil
{

public:

    static std::string getTmpFileName( const std::string& dir, const std::string& extName = ".tmp" )
    {
        time_t _time = time(NULL);
        struct tm * timeinfo;
        char buffer[80];
        timeinfo = localtime(&_time);
        std::size_t len = strftime(buffer, 80, "%Y%m%d%H%M%S", timeinfo);
        std::string time_stamp( buffer, len);

        std::string fileName = "";
        for ( uint32_t num = std::numeric_limits<uint32_t>::min(); num < std::numeric_limits<uint32_t>::max(); ++num)
        {
            fileName = time_stamp+"-"+boost::lexical_cast<std::string>(num)+extName;
            if ( !exists( dir+"/"+fileName) )
            {
                break;
            }
        }
        return fileName;
    }

    static std::string getTmpFileFullName( const std::string& dir, const std::string& extName = ".tmp" )
    {
        std::string fileName = getTmpFileName(dir, extName);
        return dir+"/"+fileName;
    }

    static void checkPath(const std::string& path)
    {
        if ( !exists(path) )
        {
            throw MiningConfigException(path+" does not exist");
        }
    }

    static void del(const std::string& path)
    {
        boost::filesystem::remove_all(path);
    }

    static void rename(const std::string& from, const std::string& to)
    {
        boost::filesystem::rename(from, to);
    }

    static void copy(const std::string& from, const std::string& to)
    {
        izenelib::util::copy_directory(boost::filesystem::path(from), boost::filesystem::path(to));
    }

    static bool exists(const std::string& path)
    {
        return boost::filesystem::exists(path);
    }

    static void createDir(const std::string& path)
    {
        if ( exists(path) )
        {
            if ( !boost::filesystem::is_directory(path) )
            {
                throw FileOperationException(path+" error when creating dir");
            }
        }
        else
        {
            bool b = boost::filesystem::create_directories(path);
            if (!b)
            {
                throw FileOperationException(path+" can not be created");
            }
            if ( !exists(path) )
            {
                throw FileOperationException(path+" can not be created");
            }
        }
    }


};
}

#endif

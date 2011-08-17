///
/// @file fcontainer.h
/// @brief load/save std container from/to file.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-08-17
///

#ifndef SF1R_FCONTAINER_H_
#define SF1R_FCONTAINER_H_

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/vector.hpp>

#include <glog/logging.h>

namespace sf1r
{

/**
 * Save @p container into file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to save
 * @param count the @p container is saved into file only when @p count is less than @p container size,
 *              then @p count is updated to @p container size
 * @param binary true for binary mode, false for text mode
 * @return true for success, false for failure
 */
template<class T> bool save_container(
    const std::string& dirPath,
    const std::string& fileName,
    const T& container,
    unsigned int& count,
    bool binary = false
)
{
    if (count >= container.size())
        return true;

    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    LOG(INFO) << "saving file: " << pathStr
              << ", element num: " << container.size();

    std::ios_base::openmode openMode = std::ios_base::out;
    if (binary)
    {
        openMode |= std::ios_base::binary;
    }

    std::ofstream ofs(pathStr.c_str(), openMode);
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << pathStr;
        return false;
    }

    try
    {
        if (binary)
        {
            boost::archive::binary_oarchive oa(ofs);
            oa << container;
        }
        else
        {
            boost::archive::text_oarchive oa(ofs);
            oa << container;
        }
    }
    catch(boost::archive::archive_exception& e)
    {
        LOG(ERROR) << "exception in boost::archive::text_oarchive or binary_oarchive: " << e.what()
                    << ", pathStr: " << pathStr;
        return false;
    }

    count = container.size();

    return true;
}

/**
 * Load @p container from file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to load
 * @param count stores @p container size
 * @param binary true for binary mode, false for text mode
 * @return true for success, false for failure
 */
template<class T> bool load_container(
    const std::string& dirPath,
    const std::string& fileName,
    T& container,
    unsigned int& count,
    bool binary = false
)
{
    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    std::ios_base::openmode openMode = std::ios_base::in;
    if (binary)
    {
        openMode |= std::ios_base::binary;
    }

    std::ifstream ifs(pathStr.c_str(), openMode);
    if (ifs)
    {
        LOG(INFO) << "start loading file : " << pathStr;

        try
        {
            if (binary)
            {
                boost::archive::binary_iarchive ia(ifs);
                ia >> container;
            }
            else
            {
                boost::archive::text_iarchive ia(ifs);
                ia >> container;
            }
        }
        catch(boost::archive::archive_exception& e)
        {
            LOG(ERROR) << "exception in boost::archive::text_iarchive or binary_iarchive: " << e.what()
                       << ", pathStr: " << pathStr;
            return false;
        }

        count = container.size();

        LOG(INFO) << "finished loading , element num: " << count;
    }

    return true;
}

}

#endif //SF1R_FCONTAINER_H_

///
/// @file fcontainer_febird.h
/// @brief using febird serialization, load/save std container from/to file.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-01
///

#ifndef SF1R_FCONTAINER_FEBIRD_H_
#define SF1R_FCONTAINER_FEBIRD_H_

#include <3rdparty/febird/io/DataIO.h>
#include <3rdparty/febird/io/StreamBuffer.h>
#include <3rdparty/febird/io/FileStream.h>
#include <boost/filesystem/path.hpp>

#include <glog/logging.h>

namespace sf1r
{

/**
 * Save @p container into file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to save
 * @return true for success, false for failure
 */
template<class T> bool save_container_febird(
    const std::string& dirPath,
    const std::string& fileName,
    const T& container)
{
    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    LOG(INFO) << "saving file: " << fileName
              << ", element num: " << container.size();

    febird::FileStream ofs(pathStr.c_str(), "w");
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << fileName;
        return false;
    }

    try
    {
        febird::NativeDataOutput<febird::OutputBuffer> ar;
        ar.attach(&ofs);
        ar & container;
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in febird::NativeDataOutput: " << e.what()
                   << ", fileName: " << fileName;
        return false;
    }

    return true;
}

/**
 * Save @p container into file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to save
 * @param count the @p container is saved into file only when @p count is less than @p container size,
 *              then @p count is updated to @p container size
 * @return true for success, false for failure
 */
template<class T> bool save_container_febird(
    const std::string& dirPath,
    const std::string& fileName,
    const T& container,
    unsigned int& count)
{
    const unsigned int containerSize = container.size();
    if (count >= containerSize)
        return true;

    if (save_container_febird(dirPath, fileName, container))
    {
        count = containerSize;
        return true;
    }

    return false;
}

/**
 * Load @p container from file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to load
 * @param count stores @p container size
 * @return true for success, false for failure
 */
template<class T> bool load_container_febird(
    const std::string& dirPath,
    const std::string& fileName,
    T& container)
{
    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    std::string pathStr = filePath.string();

    febird::FileStream ifs;
    if (! ifs.xopen(pathStr.c_str(), "r"))
        return true;

    try
    {
        febird::NativeDataInput<febird::InputBuffer> ar;
        ar.attach(&ifs);
        ar & container;
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in febird::NativeDataInput: " << e.what()
                   << ", fileName: " << fileName;
        return false;
    }

    LOG(INFO) << "finished loading file: " << fileName
              << ", element num: " << container.size();

    return true;
}

/**
 * Load @p container from file.
 * @param dirPath directory path
 * @param fileName file name
 * @param container the container to load
 * @param count stores @p container size
 * @return true for success, false for failure
 */
template<class T> bool load_container_febird(
    const std::string& dirPath,
    const std::string& fileName,
    T& container,
    unsigned int& count)
{
    if (load_container_febird(dirPath, fileName, container))
    {
        count = container.size();
        return true;
    }

    return false;
}

} // namespace sf1r

#endif //SF1R_FCONTAINER_FEBIRD_H_

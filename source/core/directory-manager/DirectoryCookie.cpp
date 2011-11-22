/**
 * @file core/directory-manager/DirectoryCookie.cpp
 * @author Ian Yang
 * @date Created <2009-10-27 10:25:35>
 * @date Updated <2009-10-28 17:00:49>
 */
#include <boost/lexical_cast.hpp>
#include "DirectoryCookie.h"

#include "DirectoryTraits.h"

namespace sf1r {
namespace directory {

const std::string DirectoryCookie::kMagicCode = "DCMG";
const std::string DirectoryCookie::kTrue = "true";
const std::string DirectoryCookie::kFalse = "false";

namespace { // {anonymous}
const unsigned kMaxCharsInLine = 4096;
const unsigned kNumLines = 4; // magic, update, parent, valid
} // namespace {anonymous}

DirectoryCookie::DirectoryCookie(const bfs::path& filePath)
    : path_(filePath.file_string())
    , updateTime_(0)
    , parentName_(DirectoryTraits::kNotName)
    , valid_(true)
    , fs_()
{
}

/**
 * read by line, may change to other data file in future
 * @todo use key-value pair instead of hard coding mapping
 */
bool DirectoryCookie::read()
{
    if (reopenForRead_())
    {
        std::string lines[kNumLines];
        for (unsigned i = 0; i != kNumLines; ++i)
        {
            if (!std::getline(fs_, lines[i]))
            {
                return false;
            }
        }

        // 0, magic
        if (lines[0] != kMagicCode)
        {
            return false;
        }

        // 1, update time
        long updateTime = 0;
        try
        {
            updateTime = boost::lexical_cast<long>(lines[1]);
        }
        catch(boost::bad_lexical_cast &)
        {
            return false;
        }

        // 3, valid
        if (lines[3] != kTrue && lines[3] != kFalse)
        {
            valid_ = false;
            return false;
        }


        // check finish, assign values
        updateTime_ = updateTime;
        parentName_ = lines[2];
        valid_ = lines[3] == kTrue;
    }

    return fs_;
}

bool DirectoryCookie::write()
{
    if (reopenForWrite_())
    {
        fs_ << kMagicCode << std::endl
            << updateTime_ << std::endl
            << parentName_ << std::endl
            << (valid_ ? kTrue : kFalse) << std::endl;
    }
    fs_.flush();
    return fs_;
}

const std::string& DirectoryCookie::parentName() const
{
    return parentName_;
}
void DirectoryCookie::setParentName(const std::string& name)
{
    parentName_ = name;
}

long DirectoryCookie::getUpdateTime() const
{
    return updateTime_;
}

void DirectoryCookie::setUpdateTime(long time)
{
    updateTime_ = time;
}

bool DirectoryCookie::invalidate()
{
    valid_ = false;
    return write();
}

bool DirectoryCookie::valid() const
{
    return valid_;
}
void DirectoryCookie::setValid(bool valid)
{
    valid_ = valid;
}

bool DirectoryCookie::reopenForRead_()
{
    if (fs_.is_open())
    {
        fs_.close();
    }

    fs_.clear();
    fs_.open(path_.c_str(), std::fstream::in);

    return fs_;
}

bool DirectoryCookie::reopenForWrite_()
{
    if (fs_.is_open())
    {
        fs_.close();
    }

    fs_.clear();
    fs_.open(path_.c_str(), std::fstream::out | std::fstream::trunc);

    return fs_;
}

}} // namespace sf1r::directory

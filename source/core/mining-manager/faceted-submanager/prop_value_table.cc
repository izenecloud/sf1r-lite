#include "prop_value_table.h"
#include <mining-manager/MiningException.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/lexical_cast.hpp>

#include <glog/logging.h>

using namespace sf1r::faceted;

namespace
{
const PropValueTable::pvid_t MAX_PV_ID = -1;

const unsigned int MAX_PV_COUNT = MAX_PV_ID + 1;

const char* PROP_STR_FILE_EXTEND = ".map";

const char* PROP_ID_FILE_EXTEND = ".table";

std::string getPathStr(
    const std::string& dirPath,
    const std::string& fileName
)
{
    boost::filesystem::path filePath(dirPath);
    filePath /= fileName;
    return filePath.string();
}

}

PropValueTable::PropValueTable(
    const std::string& dirPath,
    const std::string& propName
)
: dirPath_(dirPath)
, propName_(propName)
, savePropStrNum_(0)
, savePropIdNum_(0)
{
    // prop id 0 is reserved for empty prop value
    const izenelib::util::UString propStr;
    const pvid_t pvId = 0;

    propStrVec_.push_back(propStr);
    propStrMap_[propStr] = pvId;
    propIdVec_.push_back(pvId);
}

PropValueTable::pvid_t PropValueTable::propValueId(const izenelib::util::UString& value)
{
    pvid_t pvId = propStrVec_.size();

    typedef std::map<izenelib::util::UString, pvid_t> PropStrMap;
    std::pair<PropStrMap::iterator, bool> result = propStrMap_.insert(
        PropStrMap::value_type(value, pvId)
    );

    if (result.second)
    {
        if (pvId != 0)
        {
            propStrVec_.push_back(value);
        }
        else
        {
            // overflow
            throw MiningException(
                "property value count is out of range",
                boost::lexical_cast<std::string>(propStrVec_.size()),
                "PropValueTable::propValueId"
            );
        }
    }
    else
    {
        pvId = result.first->second;
    }

    return pvId;
}

bool PropValueTable::open()
{
    {
        std::string fileName = getPathStr(dirPath_, propName_ + PROP_STR_FILE_EXTEND);
        std::ifstream ifs(fileName.c_str());
        if (ifs)
        {
            try
            {
                boost::archive::text_iarchive ia(ifs);
                ia >> propStrVec_;
            }
            catch(boost::archive::archive_exception& e)
            {
                LOG(ERROR) << "exception in boost::archive::text_iarchive: " << e.what()
                           << ", fileName: " << fileName;
                return false;
            }

            savePropStrNum_ = propStrVec_.size();

            if (savePropStrNum_ > MAX_PV_COUNT)
            {
                LOG(ERROR) << "property value count is out of range: " << savePropStrNum_
                           << ", fileName: " << fileName;
                return false;
            }

            for (unsigned int i = 0; i < propStrVec_.size(); ++i)
            {
                propStrMap_[propStrVec_[i]] = i;
            }
        }
    }

    {
        std::string fileName = getPathStr(dirPath_, propName_ + PROP_ID_FILE_EXTEND);
        std::ifstream ifs(fileName.c_str(), std::ios_base::ate | std::ios_base::binary);
        if (ifs)
        {
            const int len = ifs.tellg();
            if (len == -1)
            {
                LOG(ERROR) << "failed on std::ifstream::tellg(), fileName: " << fileName;
                return false;
            }

            if (len > 0)
            {
                if (len % sizeof(pvid_t) != 0)
                {
                    LOG(ERROR) << "the property id file size should be muliple of id type size"
                               << ", fileName: " << fileName << ", file size: " << len;
                    return false;
                }

                const int count = len / sizeof(pvid_t);
                if (!ifs.seekg(0, std::ios_base::beg))
                {
                    LOG(ERROR) << "failed seeking to beginning of file " << fileName;
                    return false;
                }

                propIdVec_.resize(count);
                char* buf = reinterpret_cast<char*>(&propIdVec_[0]);
                if (!ifs.read(buf, len))
                {
                    LOG(ERROR) << "failed reading file " << fileName
                               << ", file size: " << len;
                    return false;
                }
                savePropIdNum_ = count;
            }
        }
    }

    LOG(INFO) << "group data loaded, property: " << propName_
              << ", id num: " << propIdVec_.size()
              << ", value num: " << propStrVec_.size();

    return true;
}

bool PropValueTable::flush()
{
    if (savePropStrNum_ < propStrVec_.size())
    {
        std::string fileName = getPathStr(dirPath_, propName_ + PROP_STR_FILE_EXTEND);
        std::ofstream ofs(fileName.c_str());
        if (! ofs)
        {
            LOG(ERROR) << "failed opening file " << fileName;
            return false;
        }

        try
        {
            boost::archive::text_oarchive oa(ofs);
            oa << propStrVec_;
        }
        catch(boost::archive::archive_exception& e)
        {
            LOG(ERROR) << "exception in boost::archive::text_iarchive: " << e.what()
                       << ", fileName: " << fileName;
            return false;
        }

        savePropStrNum_ = propStrVec_.size();
        assert(savePropStrNum_ <= MAX_PV_COUNT);
    }

    if (savePropIdNum_ < propIdVec_.size())
    {
        std::string fileName = getPathStr(dirPath_, propName_ + PROP_ID_FILE_EXTEND);
        std::ofstream ofs(fileName.c_str(), std::ios_base::app | std::ios_base::binary);
        if (! ofs)
        {
            LOG(ERROR) << "failed opening file " << fileName;
            return false;
        }

        const std::streamsize len = (propIdVec_.size() - savePropIdNum_) * sizeof(pvid_t);
        const char* buf = reinterpret_cast<const char*>(&propIdVec_[savePropIdNum_]);
        if (! ofs.write(buf, len))
        {
            LOG(ERROR) << "failed writing file " << fileName
                       << ", savePropIdNum_: " << savePropIdNum_
                       << ", propIdVec_.size(): " << propIdVec_.size();
            return false;
        }

        savePropIdNum_ = propIdVec_.size();
    }

    LOG(INFO) << "group data flushed, property: " << propName_
              << ", id num: " << savePropIdNum_
              << ", value num: " << savePropStrNum_;

    return true;
}

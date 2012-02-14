#include "fp_storage.h"
#include <boost/serialization/vector.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>

using namespace sf1r;

FpStorage::FpStorage(const std::string& dir)
    : dir_(dir)
{
}

bool FpStorage::GetFpByTableId(uint32_t id, std::vector<FpItem>& fp)
{
    fp.clear();
    std::string file = dir_ + "/" + boost::lexical_cast<std::string>(id);
    try
    {
        if (boost::filesystem::exists(file))
        {
            std::ifstream ifs(file.c_str(), std::ios::binary);
            if (ifs.fail()) return false;
            {
                boost::archive::binary_iarchive ia(ifs);
                ia >> fp ;
            }
            ifs.close();
            if (ifs.fail())
            {
                return false;
            }
        }
    }
    catch (std::exception& ex)
    {
        return false;
    }
    return true;
}

bool FpStorage::SetFpByTableId(uint32_t id, const std::vector<FpItem>& fp)
{
    std::string file = dir_ + "/" + boost::lexical_cast<std::string>(id);
    std::ofstream ofs(file.c_str());
    if ( ofs.fail()) return false;
    {
        boost::archive::binary_oarchive oa(ofs);
        oa << fp ;
    }
    ofs.close();
    if (ofs.fail())
    {
        return false;
    }
    return true;
}

#include "cmatch_generator.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/Utilities.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

CMatchGenerator::CMatchGenerator(OfferDb* odb)
:odb_(odb)
{
}


bool CMatchGenerator::Generate(const std::string& mdb_instance)
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }

    boost::unordered_set<uint128_t> changed_oid;

    //boost::filesystem::path mi_path(mdb_instance);
    //std::string ts = mi_path.filename();
    std::string scd_path = B5MHelper::GetRawPath(mdb_instance);
    std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    B5MHelper::PrepareEmptyDir(output_dir);

    std::string match_file = mdb_instance+"/match";
    if(!bfs::exists(match_file))
    {
        LOG(INFO)<<"match file not exists"<<std::endl;
        return false;
    }
    std::string cmatch_file = mdb_instance+"/cmatch";
    std::ofstream ofs(cmatch_file.c_str());
    std::ifstream ifs(match_file.c_str());
    std::string line;
    uint32_t i=0;
    while( getline(ifs, line))
    {
        boost::algorithm::trim(line);
        ++i;
        if(i%100000==0)
        {
            LOG(INFO)<<"match => cmatch processing "<<i<<" item"<<std::endl;
        }
        boost::algorithm::trim(line);
        std::vector<std::string> vec;
        boost::algorithm::split(vec, line, boost::is_any_of(","));
        if(vec.size()<2) continue;
        const std::string& soid = vec[0];
        const std::string& spid = vec[1];
        std::string old_spid;
        odb_->get(soid, old_spid);
        if(spid!=old_spid)
        {
            ofs<<soid<<","<<spid<<","<<old_spid<<std::endl;
        }
    }
    ofs.close();
    ifs.close();

    return true;
}


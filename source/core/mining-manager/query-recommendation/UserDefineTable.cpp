#include "UserDefineTable.h"

#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>

namespace sf1r
{
namespace Recommend
{
static std::string uuid = "UserDefineTable";
static std::string timestamp = ".UserDefineTimestamp";

UserDefineTable::UserDefineTable(const std::string& workdir)
    : timestamp_(0)
    , workdir_(workdir)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    
    {
        std::string path = workdir_;
        path += "/";
        path += timestamp;

        std::ifstream in;
        in.open(path.c_str(), std::ifstream::in);
        in>>timestamp_;
        in.close();
    }
    
    {
        std::string path = workdir_;
        path += "/";
        path += uuid;
        if (boost::filesystem::exists(path.c_str()))
        {
            boost::filesystem::path p(path);
            if ((0 < timestamp_) && (timestamp_ >= boost::filesystem::last_write_time(p)))
            {
                std::ifstream ifs(path.c_str(), std::ios::binary);
                boost::archive::text_iarchive ia(ifs);
                ia >> table_;
                ifs.close();
            }
        }
    }
}

UserDefineTable::~UserDefineTable()
{
}


void UserDefineTable::build(const std::string& path)
{
    //if (!isNeedBuild)
    //    return;
    //clear();

    std::string resource;
    if (!boost::filesystem::exists(path))
        resource = workdir_;
    else if(!boost::filesystem::is_directory(path))
        resource = workdir_;
    else
        resource = path;
    
    try
    {
        if (!boost::filesystem::exists(resource))
            return ;
        else if(boost::filesystem::is_directory(resource))
        {
            boost::filesystem::directory_iterator end;
            for(boost::filesystem::directory_iterator it(resource) ; it != end ; ++it)
            {
                const std::string p = it->path().string();
                if(boost::filesystem::is_regular_file(p))
                {
                    if (isValid(p))
                        buildFromFile(p);
                }
            }
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        std::cout << ex.what() << '\n';
    }
    timestamp_ = time(NULL);
}

bool UserDefineTable::isNeedBuild(const std::string& path) const
{
    if (timestamp_ == 0)
        return true;

    std::string resource;
    if (!boost::filesystem::exists(path))
        resource = workdir_;
    else if(!boost::filesystem::is_directory(path))
        resource = workdir_;
    else
        resource = path;
    
    try
    {
        if (!boost::filesystem::exists(resource))
            return false;
        else if(boost::filesystem::is_directory(resource))
        {
            boost::filesystem::directory_iterator end;
            for(boost::filesystem::directory_iterator it(resource) ; it != end ; ++it)
            {
                const std::string p = it->path().string();
                if(boost::filesystem::is_regular_file(p))
                {
                    if (timestamp_ < boost::filesystem::last_write_time(it->path()))
                        return true;
                }
            }
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        std::cout << ex.what() << '\n';
    }
    return false;
}


bool UserDefineTable::isValid(const std::string& f) const
{
    const boost::filesystem::path p(f);
    return ".udef" == boost::filesystem::extension(p);
}
    
void UserDefineTable::buildFromFile(const std::string& f)
{
    std::ifstream ifs(f.c_str());
    int maxLine = 102400;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);

    while(ifs.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);

        std::size_t tabPos = sLine.find('\t');
        if (std::string::npos == tabPos)
            continue;
        std::string key = sLine.substr(0, tabPos);
        std::string value = sLine.substr(tabPos + 1);
        table_[key] = value;
    }
    ifs.close();
}

bool UserDefineTable::search(const std::string& userQuery, std::string& correct) const
{
    boost::unordered_map<std::string, std::string>::const_iterator it = table_.find(userQuery);
    if (table_.end() == it)
        return false;
    correct = it->second;
    return true;
}

void UserDefineTable::flush() const
{
    std::string path = workdir_;
    path += "/";
    path += uuid;
    std::ofstream ofs(path.c_str(), std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    oa << table_;
    ofs.close();
}

void UserDefineTable::clear()
{
    table_.clear();
}

}
}

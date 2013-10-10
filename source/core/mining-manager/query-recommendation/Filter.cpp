#include "Filter.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <time.h>

namespace sf1r
{
namespace Recommend
{

static std::string uuid = ".Filter";
static std::string timestamp = ".FilterTimestamp";

Filter::Filter(const std::string& workdir)
    : timestamp_(0)
    , workdir_(workdir)
{
    bf_ = new BloomFilter(10240, 1e-8, 1024);
    
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    in>>*this;
    in.close();

    path = workdir_;
    path += "/";
    path += timestamp;

    in.open(path.c_str(), std::ifstream::in);
    in>>timestamp_;
    in.close();
}

Filter::~Filter()
{
    if (NULL != bf_)
    {
        delete bf_;
        bf_ = NULL;
    }
}

bool Filter::isNeedBuild() const
{
    try
    {
        if (!boost::filesystem::exists(workdir_))
            return false;
        else if(boost::filesystem::is_directory(workdir_))
        {
            boost::filesystem::directory_iterator end;
            for(boost::filesystem::directory_iterator it(workdir_) ; it != end ; ++it)
            {
                const std::string p = it->path().string();
                if(boost::filesystem::is_regular_file(p))
                {
                    if (isValid(p))
                    {
                        std::time_t mt = boost::filesystem::last_write_time(it->path());
                        if (mt > timestamp_)
                            return true;
                    }
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

void Filter::buildFilter()
{
    try
    {
        if (!boost::filesystem::exists(workdir_))
            return;
        else if(boost::filesystem::is_directory(workdir_))
        {
            boost::filesystem::directory_iterator end;
            for(boost::filesystem::directory_iterator it(workdir_) ; it != end ; ++it)
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

bool Filter::isValid(const std::string& f) const
{
    const boost::filesystem::path p(f);
    return ".filter" == boost::filesystem::extension(p);
}

void Filter::buildFromFile(const std::string& f)
{
    std::ifstream in;
    in.open(f.c_str(), std::ifstream::in);
    int maxLine = 1024;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);

    while(in.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);

        boost::to_lower(sLine);
        boost::trim(sLine);
        //std::cout<<sLine<<"\n";
        bf_->Insert(sLine);
    }
}

bool Filter::isFilter(const std::string& userQuery) const
{
    std::string sLine(userQuery);
    boost::to_lower(sLine);
    boost::trim(sLine);
    return bf_->Get(sLine);
}

void Filter::clear()
{
    if (NULL != bf_)
    {
        delete bf_;
        bf_ = NULL;
    }
    bf_ = new BloomFilter(10240, 1e-8, 1024);
    
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out.close();
}

void Filter::flush() const
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
    out.close();
    
    path = workdir_;
    path += "/";
    path += timestamp;

    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<timestamp_;
    out.close();
}

std::ostream& operator<<(std::ostream& out, const Filter& filter)
{
    filter.bf_->save(out);
    return out;
}

std::istream& operator>>(std::istream& in,  Filter& filter)
{
    filter.bf_->load(in);
    return in;
}

}
}

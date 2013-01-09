#include "ScdControlRecevier.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>


using namespace sf1r;
ScdControlRecevier::ScdControlRecevier(const std::string& syncID, 
                            const std::string& collectionName,
                            const std::string& controlFilePath)
                            : syncID_(syncID)
                            , collectionName_(collectionName)
                            , controlFilePath_(controlFilePath)
{
    syncConsumer_ = SynchroFactory::getConsumer(syncID);
    syncConsumer_->watchProducer(
            boost::bind(&ScdControlRecevier::Run, this, _1),
            collectionName);
}

bool ScdControlRecevier::Run(const std::string& scd_control_path)
{
    LOG(INFO) << "ScdControlRecevier::Run " << scd_control_path << std::endl;
    if (!scd_control_path.empty())
    {
        boost::filesystem::path fromPath(scd_control_path);//file
        boost::filesystem::path toPath(controlFilePath_);// file , not dietcionay...
        if (boost::filesystem::exists(toPath))
        {
            boost::filesystem::remove(toPath);
        }
        if (boost::filesystem::exists(fromPath))
        {
            boost::filesystem::copy_file(fromPath, toPath);
        }
        cout<<"RUN OVER"<<endl;
    }
    else
        return false;
    return true;
}
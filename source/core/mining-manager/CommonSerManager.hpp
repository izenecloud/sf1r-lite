///
/// @file   CommonSerManager.hpp
/// @brief  the super class of all mining component which has at least one ser data.
/// @author Jia Guo
/// @date   2010-02-04
/// @date   2010-02-04
///
#ifndef COMMONSERMANAGER_HPP_
#define COMMONSERMANAGER_HPP_
#include <boost/filesystem.hpp>
#include <fstream>

#include <am/tokyo_cabinet/tc_hash.h>
namespace sf1r
{
template<typename INFO_TYPE, bool SINGLE_FILE=false>
class CommonSerManager
{

public:
    
    public:
        
        
        CommonSerManager(const std::string& path) :
        path_(path), isOpen_(false), 
        info_(), serInfo_(NULL)
        {
            
        }
        
        virtual ~CommonSerManager()
        {
            close();
        }
        
        bool isOpen()
        {
            return isOpen_;
        }
        
        void open()
        {
            if( !isOpen() )
            {
                std::string ser_path = path_;
                if( !SINGLE_FILE )
                {
                    boost::filesystem::create_directories(path_);
                    ser_path = path_+"/ser_info";
                }
                serInfo_ = new izenelib::am::tc_hash<bool, INFO_TYPE >(ser_path);
                serInfo_->setCacheSize(1);
                serInfo_->open();
                INFO_TYPE data;
                if( !serInfo_->get(0, data) )
                {
                    serInfo_->insert(0, data);
                }
                if( SINGLE_FILE )
                {
                    isOpen_ = true;
                }
            }
        }
        
        void flush()
        {
            if( isOpen() )
            {
                serInfo_->flush();
            }
        }
        
        void close()
        {
            if( isOpen() )
            {
                flush();
                delete serInfo_;
                serInfo_ = NULL;
                if( SINGLE_FILE )
                {
                    isOpen_ = false;
                }
            }
        }
        
        std::string getPath()
        {
            return path_;
        }

        
        void setSerData(const INFO_TYPE& data)
        {
            serInfo_->update(0, data);
        }
        
        void getSerData(INFO_TYPE& data)
        {
            serInfo_->get(0, data);
        }
        
                
        void del()
        {
            close();
            boost::filesystem::remove_all(path_);
        }
        
        
        
    private:
        std::string path_;
        bool isOpen_;
        INFO_TYPE info_;
        izenelib::am::tc_hash<bool, INFO_TYPE >* serInfo_;
        
        
};
    
}
#endif

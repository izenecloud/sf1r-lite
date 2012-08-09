#ifndef IMAGE_SERVER_STORAGE_H_
#define IMAGE_SERVER_STORAGE_H_

#include <util/singleton.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/unordered_map.hpp>
//#include <3rdparty/am/luxio/btree.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <am/tokyo_cabinet/tc_hash.h>
#include <tfs_client_api.h>
#include <tblog.h>

#define MAX_RET_LEN  8

using namespace tfs::client;
using namespace tfs::common;

namespace sf1r
{

class ImageServerStorage
{
public:
    typedef std::string KeyType ;
    typedef std::string ValueType;

    //typedef Lux::IO::Btree ImageColorDBT;
    typedef izenelib::am::tc_hash<KeyType, ValueType>  ImageColorDBT;
public:
    static ImageServerStorage* get()
    {
        return izenelib::util::Singleton<ImageServerStorage>::get();
    }
    ImageServerStorage()
        :img_color_db_(NULL)
         , tfsclient(NULL)
    {
    }

    bool init(const std::string& img_color_db_path, const std::string& img_file_server)
    {
        if(img_color_db_)
            return true;
        img_color_db_ = new ImageColorDBT(img_color_db_path);
        if(!open())
        {
            std::cerr << "Failed to open image color db." << std::endl;
            delete img_color_db_;
            img_color_db_ = NULL;
            return false;
        }
        img_color_db_path_ = img_color_db_path;

        TBSYS_LOGGER.setLogLevel("INFO");
        tfsclient = TfsClient::Instance();
        int ret = tfsclient->initialize(img_file_server.c_str());
        if(ret != TFS_SUCCESS)
        {
            std::cerr << "Error: connect to image file server failed." << std::endl;
            tfsclient = NULL;
            return false;
        }

        return true;
    }

    void close()
    {
        if (img_color_db_)
        {
            img_color_db_->close();
            delete img_color_db_;
            img_color_db_ = NULL;
        }
    }

    bool open()
    {
        try
        {
            return img_color_db_->open();
        }
        catch (...)
        {
            return false;
        }
        return true;
    }


    /* -----------------------------------*/
    /**
     * @Brief  used for copy current image color db to a backup db.
     * during the backup, make sure there is no write on the db file. 
     *
     * @Param backup_suffix, the extra suffix name append to the current db path.
     *
     * @Returns  return true if backup success, otherwise return false. 
     */
    /* -----------------------------------*/
    bool backup_imagecolor_db(const std::string& backup_suffix)
    {
        if(img_color_db_)
        {
            try
            {
                img_color_db_->flush();
                boost::filesystem::copy_file(img_color_db_path_, img_color_db_path_ + backup_suffix, 
                    boost::filesystem::copy_option::overwrite_if_exists);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }
        return false;
    }

    bool GetImageColor(const KeyType& img_path, std::string& ret)
    {
        if(img_color_db_)
        {
            try
            {
                return img_color_db_->get(img_path, ret);
            }
            catch(izenelib::util::IZENELIBException& e)
            {
                std::cerr << "get image color exception : " << e.what() << endl;
            }
        }
        return false;
    }

    bool SetImageColor(const KeyType& img_path, const ValueType& value)
    {
        if(img_color_db_)
        {
            try
            {
                return img_color_db_->update(img_path, value);
            }
            catch(izenelib::util::IZENELIBException& e)
            {
                std::cerr << "set image color exception : " << e.what() << endl;
            }
        }
        return false;
    }

    bool UploadImageFile(const std::string& img_local_file, std::string& ret_file_name)
    {
        if(tfsclient)
        {
            char tmp_name[TFS_FILE_LEN];
            int64_t ret_size = tfsclient->save_file(tmp_name, TFS_FILE_LEN, img_local_file.c_str(), T_DEFAULT, NULL, NULL);
            if(ret_size <= 0)
            {
                return false;
            }
            ret_file_name = std::string(tmp_name);
            return true;
        }
        return false;
    }
    bool UploadImageData(const char* img_data, std::size_t data_size, std::string& ret_file_name)
    {
        if(tfsclient)
        {
            char tmp_name[TFS_FILE_LEN];
            int64_t ret_size = tfsclient->save_buf(tmp_name, TFS_FILE_LEN,
                img_data, data_size, T_DEFAULT, NULL, NULL, NULL);
            if(ret_size < (int64_t)data_size)
            {
                return false;
            }
            ret_file_name = std::string(tmp_name);
            return true;
        }
        return false;
    }
    bool DeleteImage(const std::string& storage_img_name)
    {
        if(tfsclient)
        {
            int64_t file_size;
            int ret = tfsclient->unlink(file_size, storage_img_name.c_str(), NULL);
            if(ret != TFS_SUCCESS)
            {
                return false;
            }
            return true;
        }
        return false;
    }
    bool DownloadImageData(const std::string& storage_img_name, char*& buffer, std::size_t& buf_size)
    {
        assert(buffer == NULL);
        int fd = -1;
        int ret = 0;
        fd = tfsclient->open(storage_img_name.c_str(), NULL, T_READ);
        if( fd <= 0 )
        {
            std::cerr << "tfs open file failed : " << storage_img_name << std::endl;
            return false;
        }
        TfsFileStat fstat;
        ret = tfsclient->fstat(fd, &fstat);
        if( ret != TFS_SUCCESS || fstat.size_ <= 0)
        {
            buf_size = 0;
            return false;
        }

        buf_size = fstat.size_;
        buffer = new char[buf_size];
        std::size_t readed = 0;
        while(readed < buf_size)
        {
            ret = tfsclient->read(fd, buffer + readed, buf_size - readed);
            if(ret < 0)
            {
                break;
            }
            else
            {
                readed += ret;
            }
        }
        if(ret < 0)
        {
            tfsclient->close(fd);
            delete[] buffer;
            buf_size = 0;
            return false;
        }
        tfsclient->close(fd);
        return true;
    }

private:

    std::string   img_color_db_path_;
    ImageColorDBT*  img_color_db_;
    TfsClient*      tfsclient;
};

}

#endif /* IMAGE_SERVER_STORAGE_H_ */

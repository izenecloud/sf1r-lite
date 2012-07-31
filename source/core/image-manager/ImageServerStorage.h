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


#define MAX_RET_LEN  8

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
    {
    }

    bool init(const std::string& img_color_db_path)
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
            img_color_db_->flush();
            try
            {
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
            return img_color_db_->get(img_path, ret);
        }
        return false;
    }

    bool SetImageColor(const KeyType& img_path, const ValueType& value)
    {
        if(img_color_db_)
        {
            return img_color_db_->update(img_path, value);
        }
        return false;
    }

private:

    std::string   img_color_db_path_;
    ImageColorDBT*  img_color_db_;
};

}

#endif /* IMAGE_SERVER_STORAGE_H_ */

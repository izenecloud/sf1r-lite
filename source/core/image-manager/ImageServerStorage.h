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

    ImageColorDBT*  img_color_db_;
};

}

#endif /* IMAGE_SERVER_STORAGE_H_ */

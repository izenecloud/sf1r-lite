#ifndef IMAGE_SERVER_STORAGE_H_
#define IMAGE_SERVER_STORAGE_H_

#include <util/singleton.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/unordered_map.hpp>
#include <3rdparty/am/luxio/btree.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <string>

#define MAX_RET_LEN  8

namespace sf1r
{

class ImageServerStorage
{
public:
    typedef std::string KeyType ;
    typedef std::string ValueType;

    typedef Lux::IO::Btree ImageColorDBT;
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
        img_color_db_ = new ImageColorDBT(Lux::IO::NONCLUSTER);
        img_color_db_->set_noncluster_params(Lux::IO::Linked);
        img_color_db_->set_lock_type(Lux::IO::LOCK_THREAD);
        if(!open(img_color_db_path))
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

    bool open(const std::string& path)
    {
        try
        {
            if ( !boost::filesystem::exists( path ) )
            {
                img_color_db_->open(path.c_str(), Lux::IO::DB_CREAT);
            }
            else
            {
                img_color_db_->open(path.c_str(), Lux::IO::DB_RDWR);
            }
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
            Lux::IO::data_t key = {img_path.c_str(), img_path.size()};
            Lux::IO::data_t *val = NULL;
            if(!img_color_db_->get(&key, &val, Lux::IO::SYSTEM) || val == NULL || val->data == NULL)
            {
                return false;
            }
            ret.assign((const char*)val->data, val->size);
            img_color_db_->clean_data(val);
            return true;
        }
        return false;
    }

    bool SetImageColor(const KeyType& img_path, const ValueType& value)
    {
        if(img_color_db_)
        {
            Lux::IO::data_t key = {img_path.c_str(), img_path.size()};
            Lux::IO::data_t val = {value.c_str(), value.size()};
            return img_color_db_->put(&key, &val); // insert operation
        }
        return false;
    }

private:

    ImageColorDBT*  img_color_db_;
};

}

#endif /* IMAGE_SERVER_STORAGE_H_ */

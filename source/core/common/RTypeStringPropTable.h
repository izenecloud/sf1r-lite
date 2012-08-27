#ifndef SF1R_COMMON_RTYPESTRING_PROPERTY_TABLE_H
#define SF1R_COMMON_RTYPESTRING_PROPERTY_TABLE_H

#include <common/type_defs.h>
#include <common/PropertyValue.h>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <string.h>
#include <3rdparty/am/luxio/array.h>
#include <string>
#include <boost/filesystem.hpp>
#include <stdint.h>
#include <memory.h>

namespace sf1r
{

class RTypeStringPropTable 
{
public:
    RTypeStringPropTable(PropertyDataType type)
        : type_(type), data_(NULL)
    {
        data_ = new Lux::IO::Array(Lux::IO::NONCLUSTER);
        data_->set_noncluster_params(Lux::IO::Linked);
        data_->set_lock_type(Lux::IO::LOCK_THREAD);
    }

    ~RTypeStringPropTable()
    {
        if(data_)
        {
            data_->close();
            delete data_;
        }
    }

    bool init(const std::string& path)
    {
        path_ = path;
        try
        {
            if ( !boost::filesystem::exists(path_) )
            {
                data_->open(path_.c_str(), Lux::IO::DB_CREAT);
            }
            else
            {
                data_->open(path_.c_str(), Lux::IO::DB_RDWR);
            }
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    bool getRTypeString(const unsigned int docId, std::string& rtype_value) const
    {
        Lux::IO::data_t *val_p = NULL;
        if (!data_->get(docId, &val_p, Lux::IO::SYSTEM))
        {
            data_->clean_data(val_p);
            return false;
        }

        if ( val_p->size == 0 )
        {
            data_->clean_data(val_p);
            return false;
        }

        char* p = new char[val_p->size];
        memcpy(p, val_p->data, val_p->size);
        rtype_value.assign(p, val_p->size);

        delete[] p;
        data_->clean_data(val_p);
        return true;
    }

    bool exist(const unsigned int docId)
    {
        Lux::IO::data_t *val_p = NULL;
        bool ret = data_->get(docId, &val_p, Lux::IO::SYSTEM);
        data_->clean_data(val_p);
        return ret;
    }

    bool delRTypeString(const unsigned int docId)
    {
        return data_->del(docId);
    }

    bool updateRTypeString(const unsigned int docId, const std::string& rtype_value)
    {
        if ( 0 == rtype_value.size() )
        {
            return false;
        }
        Lux::IO::insert_mode_t flag = Lux::IO::APPEND;
        if(exist(docId))
            flag = Lux::IO::OVERWRITE;
        return data_->put(docId, rtype_value.data(), rtype_value.size(), flag);
    }

    bool copyValue(unsigned int docId_from, unsigned int docId_to)
    {
        std::string result;
        if(getRTypeString(docId_from, result))
        {
            return updateRTypeString(docId_to, result);
        }
        return false;
    }

protected:
    PropertyDataType type_;
    std::string path_;
    Lux::IO::Array* data_;
};

}

#endif

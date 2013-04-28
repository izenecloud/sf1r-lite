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
#include <map>
#include <limits>
#include <glog/logging.h>

#include "PropSharedLock.h"

namespace sf1r
{

class RTypeStringPropTable : public PropSharedLock
{
    typedef std::map<std::size_t, std::string> RTypeMap;
public:
    RTypeStringPropTable(PropertyDataType type)
        : type_(type)
        , data_(NULL)
        , sortEnabled_(false)
        , maxDocId_(0)
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

    void flush()
    {
        Lux::IO::insert_mode_t flag = Lux::IO::APPEND;
        if(exist(0))
            flag = Lux::IO::OVERWRITE;
        data_->put(0, &maxDocId_, sizeof(maxDocId_), flag);
    }

    PropertyDataType getType() const
    {
        return type_;
    }

    void enableSort()
    {
        ScopedWriteBoolLock(mutex_, true);
        sortEnabled_ = true;
        load_();
    }
    
    std::size_t size(bool isLock = true) const
    {
        return maxDocId_;
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
        
        Lux::IO::data_t *val_p = NULL;
        if (!data_->get(0, &val_p, Lux::IO::SYSTEM))
        {
            data_->clean_data(val_p);
            return false;
        }

        if ( val_p->size != sizeof(maxDocId_) )
        {
            data_->clean_data(val_p);
            return false;
        }

        memcpy(&maxDocId_, val_p->data, val_p->size);

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
        {
            ScopedWriteBoolLock lock(mutex_, true);
            if (sortEnabled_)
            {
                rtypeMap_[docId]=rtype_value;
            }
            if (docId > maxDocId_)
            {
                maxDocId_ = docId;
            }
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

    int compareValues(std::size_t lhs, std::size_t rhs, bool isLock)
    {
        ScopedReadBoolLock lock(mutex_, isLock);
        if ( lhs > maxDocId_ || rhs > maxDocId_ )
        {
            return -1;
        }
        std::string lv, rv;
        RTypeMap::const_iterator it = rtypeMap_.find(lhs);
        if (it == rtypeMap_.end())
        {
            return -1;
        }
        lv = it->second;
        it = rtypeMap_.find(rhs);
        if (it == rtypeMap_.end())
        { 
            return -1;
        }
        rv = it->second;
        if (lv < rv ) return -1;
        if (lv > rv ) return 1;
        return 0;
    }
private:
    void load_()
    {
        rtypeMap_.clear();

        for (unsigned int docId = 0; docId < maxDocId_; docId++)
        {
            std::string value;
            if (getRTypeString(docId, value))
            {
                rtypeMap_[docId]=value;
            }
        }
    }
protected:
    PropertyDataType type_;
    std::string path_;
    Lux::IO::Array* data_;
    RTypeMap rtypeMap_;
    bool sortEnabled_;
    unsigned int maxDocId_;
};

}

#endif

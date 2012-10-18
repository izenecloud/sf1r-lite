#ifndef SF1R_COMMON_NUMERIC_PROPERTY_TABLE_H
#define SF1R_COMMON_NUMERIC_PROPERTY_TABLE_H

#include "NumericPropertyTableBase.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace sf1r
{

template <class T>
class NumericPropertyTable : public NumericPropertyTableBase
{
public:
    NumericPropertyTable(PropertyDataType type)
        : NumericPropertyTableBase(type)
        , dirty_(false)
        , invalidValue_(std::numeric_limits<T>::max())
        , data_(1, invalidValue_)
    {
    }

    NumericPropertyTable(PropertyDataType type, const T& initValue)
        : NumericPropertyTableBase(type)
        , dirty_(false)
        , invalidValue_(initValue)
        , data_(1, invalidValue_)
    {
    }

    ~NumericPropertyTable()
    {
    }

    void init(const std::string& path)
    {
        path_ = path;
        std::ifstream ifs(path_.c_str());
        if (ifs) load_(ifs);
    }

    void resize(std::size_t size)
    {
        WriteLock lock(mutex_);
        data_.resize(size, invalidValue_);
    }

    std::size_t size() const
    {
        ReadLock lock(mutex_);
        return data_.size();
    }

    void flush()
    {
        if (!dirty_) return;
        dirty_ = false;
        std::vector<T>(data_).swap(data_);
        if (path_.empty()) return;
        std::ofstream ofs(path_.c_str());
        if (ofs) save_(ofs);
    }

    bool isValid(std::size_t pos) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;
        return true;
    }

    bool getInt32Value(std::size_t pos, int32_t& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = static_cast<int32_t>(data_[pos]);
        return true;
    }
    bool getFloatValue(std::size_t pos, float& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = static_cast<float>(data_[pos]);
        return true;
    }
    bool getInt64Value(std::size_t pos, int64_t& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = static_cast<int64_t>(data_[pos]);
        return true;
    }
    bool getDoubleValue(std::size_t pos, double& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = static_cast<double>(data_[pos]);
        return true;
    }
    bool getStringValue(std::size_t pos, std::string& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = boost::lexical_cast<std::string>(data_[pos]);
        return true;
    }
    bool getFloatPairValue(std::size_t pos, std::pair<float, float>& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value.first = value.second = static_cast<float>(data_[pos]);
        return true;
    }

    bool getValue(std::size_t pos, T& value) const
    {
        ReadLock lock(mutex_);
        if (pos >= data_.size() || data_[pos] == invalidValue_)
            return false;

        value = data_[pos];
        return true;
    }

    void* getValueList()
    {
        if (!data_.empty())
            return static_cast<void*>(&data_[0]);
        else
            return NULL;
    }
    const void* getValueList() const
    {
        if (!data_.empty())
            return static_cast<const void*>(&data_[0]);
        else
            return NULL;
    }

    void setInt32Value(std::size_t pos, const int32_t& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        data_[pos] = static_cast<T>(value);
        dirty_ = true;
    }
    void setFloatValue(std::size_t pos, const float& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        data_[pos] = static_cast<T>(value);
        dirty_ = true;
    }
    void setInt64Value(std::size_t pos, const int64_t& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        data_[pos] = static_cast<T>(value);
        dirty_ = true;
    }
    void setDoubleValue(std::size_t pos, const double& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        data_[pos] = static_cast<T>(value);
        dirty_ = true;
    }
    bool setStringValue(std::size_t pos, const std::string& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        try
        {
            data_[pos] = boost::lexical_cast<T>(value);
            dirty_ = true;
        }
        catch (const boost::bad_lexical_cast &)
        {
            return false;
        }
        return true;
    }

    void setValue(std::size_t pos, const T& value)
    {
        WriteLock lock(mutex_);
        if (pos >= data_.size())
            data_.resize(pos + 1, invalidValue_);

        data_[pos] = value;
        dirty_ = true;
    }

    void copyValue(std::size_t from, std::size_t to)
    {
        WriteLock lock(mutex_);
        if (from >= data_.size() || data_[from] == invalidValue_)
            return;

        if (to >= data_.size())
            data_.resize(to + 1, invalidValue_);

        data_[to] = data_[from];
        dirty_ = true;
    }

    int compareValues(std::size_t lhs, std::size_t rhs) const
    {
        ReadLock lock(mutex_);
        const T& lv = data_[lhs];
        if (lv == invalidValue_) return -1;
        const T& rv = data_[rhs];
        if (rv == invalidValue_) return 1;
        if (lv < rv) return -1;
        if (lv > rv) return 1;
        return 0;
    }

    void clearValue(std::size_t pos)
    {
        WriteLock lock(mutex_);
        if (pos < data_.size())
        {
            data_[pos] = invalidValue_;
            dirty_ = true;
        }
    }

protected:
    void load_(std::istream& is)
    {
        WriteLock lock(mutex_);
        std::size_t len = 0;
        is.read((char*)&len, sizeof(len));
        data_.resize(len);
        is.read((char*)&data_[0], sizeof(T) * len);
    }

    void save_(std::ostream& os) const
    {
        ReadLock lock(mutex_);
        std::size_t len = data_.size();
        os.write((const char*)&len, sizeof(len));
        os.write((const char*)&data_[0], sizeof(T) * len);
    }

protected:
    bool dirty_;
    T invalidValue_;
    std::string path_;
    std::vector<T> data_;
};

template <>
inline bool NumericPropertyTable<int64_t>::getStringValue(std::size_t pos, std::string& value) const
{
    ReadLock lock(mutex_);
    if (pos >= data_.size() || data_[pos] == invalidValue_)
        return false;

    using namespace boost::posix_time;
    if (type_ == DATETIME_PROPERTY_TYPE)
    {
        value = to_iso_string(from_time_t(data_[pos] - timezone));
    }
    else
    {
        value = boost::lexical_cast<std::string>(data_[pos]);
    }
    return true;
}

template <>
inline bool NumericPropertyTable<float>::getStringValue(std::size_t pos, std::string& value) const
{
    ReadLock lock(mutex_);
    if (pos >= data_.size() || data_[pos] == invalidValue_)
        return false;

    static std::stringstream ss;
    ss << fixed << setprecision(2) << data_[pos];
    value = ss.str();
    ss.str(std::string());
    return true;
}

template <>
inline bool NumericPropertyTable<int8_t>::setStringValue(std::size_t pos, const std::string& value)	
{
    WriteLock lock(mutex_);
    if (pos >= data_.size())
        data_.resize(pos + 1, invalidValue_);

    try
    {
        data_[pos] = boost::numeric_cast<int8_t>(boost::lexical_cast<int32_t>(value));
        dirty_ = true;
    }
    catch (const boost::bad_lexical_cast &)
    {
        return false;
    }
    return true;
}


}

#endif

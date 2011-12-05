#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_H

#include <common/type_defs.h>

#include <am/bitmap/Ewah.h>

#include <map>
#include <string>

namespace sf1r
{

/// @brief This class implements Summarization that stores a document identifier and
/// a list of summarization properties.
class Summarization
{
    typedef std::map<std::string, UString> property_named_map;
    typedef property_named_map::iterator property_mutable_iterator;
    typedef izenelib::am::EWAHBoolArray<uint32_t> fingerprint_type;
public:
    typedef property_named_map::const_iterator property_const_iterator;
    typedef property_named_map::iterator property_iterator;

    Summarization()
            : propertyList_()
    {
    }

    Summarization(const std::vector<uint32_t>& ids)
            : propertyList_()
    {
        std::vector<uint32_t>::const_iterator it = ids.begin();
        for(;it != ids.end(); ++it)
        {
            fingerPrint_.set(*it);
        }
    }

    UString& property(const std::string& propertyName)
    {
        return propertyList_[propertyName];
    }
    const UString property(const std::string& propertyName) const
    {
        property_const_iterator found = findProperty(propertyName);
        if (found != propertyEnd())
        {
            return found->second;
        }

        return UString();
    }

    /// Insert a new property into the document.
    /// @return \c true if successful, \c false if already existed
    bool insertProperty(const std::string& propertyName,
                        const UString& propertyValue)
    {
        return propertyList_.insert(
                   std::make_pair(propertyName, propertyValue)
               ).second;
    }

    /// Set a new value to a property of the document.
    void updateProperty(const std::string& propertyName,
                        UString propertyValue)
    {
        propertyList_[propertyName].swap(propertyValue);
    }

    /// Remove a property from the document. Return false if the property
    /// can not be removed.
    /// @return true if successful, false otherwise
    bool eraseProperty(const std::string& propertyName)
    {
        return propertyList_.erase(propertyName);
    }

    property_iterator findProperty(const std::string& propertyName)
    {
        return propertyList_.find(propertyName);
    }
    property_const_iterator findProperty(const std::string& propertyName) const
    {
        return propertyList_.find(propertyName);
    }

    bool hasProperty(const std::string& pname) const
    {
        property_const_iterator it = findProperty(pname);
        return it!=propertyEnd() ;
    }

    property_iterator propertyBegin()
    {
        return propertyList_.begin();
    }
    property_iterator propertyEnd()
    {
        return propertyList_.end();
    }
    property_const_iterator propertyBegin() const
    {
        return propertyList_.begin();
    }
    property_const_iterator propertyEnd() const
    {
        return propertyList_.end();
    }

    void clear()
    {
        propertyList_.clear();
    }

    bool isEmpty()
    {
        return ( propertyList_.size() == 0 );
    }

    void swap(Summarization& rhs)
    {
        using std::swap;
        swap(propertyList_, rhs.propertyList_);
        swap(fingerPrint_, rhs.fingerPrint_);
    }

    size_t getPropertySize() const
    {
        return propertyList_.size();
    }

    bool operator==(const Summarization & x) const
    {
        return fingerPrint_ == x.fingerPrint_;
    }

    bool operator!=(const Summarization & x) const
    {
        return fingerPrint_ != x.fingerPrint_;
    }

private:
    /// list of doc ids this summarization is built based on
    fingerprint_type fingerPrint_;

    /// list of properties of the document
    std::map<std::string, UString> propertyList_;

    template<class DataIO>
    friend void DataIO_loadObject(DataIO& dio, Summarization& x);
    template<class DataIO>
    friend void DataIO_saveObject(DataIO& dio, const Summarization& x);
};

inline void swap(Summarization& a, Summarization& b)
{
    a.swap(b);
}

template<class DataIO>
inline void DataIO_loadObject(DataIO& dio, Summarization& x)
{
    dio & x.fingerPrint_;
    dio & x.propertyList_;
}
template<class DataIO>
inline void DataIO_saveObject(DataIO& dio, const Summarization& x)
{
    dio & x.fingerPrint_;
    dio & x.propertyList_;
}

} // namespace sf1r

MAKE_FEBIRD_SERIALIZATION( sf1r::Summarization )

#endif // SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_H

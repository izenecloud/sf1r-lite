#ifndef SF1V5_DOCUMENT_MANAGER_DOCUMENT_H
#define SF1V5_DOCUMENT_MANAGER_DOCUMENT_H
/**
 * @file sf1r/document-manager/Document.h
 * @date Created <2009-10-20 16:12:04>
 * @date Updated <2009-10-22 03:04:08>
 */
#include <common/type_defs.h>
#include <common/PropertyValue.h>

#include <boost/any.hpp>
#include <boost/serialization/access.hpp>

#include <map>
#include <string>

namespace sf1r
{

/// @brief This class implements Document that stores a document identifier and
/// a list of document properties.
class Document
{
    typedef std::map<std::string, PropertyValue> property_named_map;
    typedef property_named_map::iterator property_mutable_iterator;

public:
    typedef property_named_map::const_iterator property_const_iterator;
    typedef property_named_map::iterator property_iterator;

    Document()
            : id_(0)
            , propertyList_()
    {
    }

    /// Construct a document with predefined document identifier
    explicit Document(docid_t id)
            : id_(id)
            , propertyList_()
    {
    }

    docid_t getId() const
    {
        return id_;
    }
    void setId(docid_t id)
    {
        id_ = id;
    }

    PropertyValue& property(const std::string& propertyName)
    {
        return propertyList_[propertyName];
    }
    const PropertyValue property(const std::string& propertyName) const
    {
        property_const_iterator found = findProperty(propertyName);
        if (found != propertyEnd())
        {
            return found->second;
        }

        return PropertyValue();
    }

    /// Insert a new property into the document.
    /// @return \c true if successful, \c false if already existed
    bool insertProperty(const std::string& propertyName,
                        const PropertyValue& propertyValue)
    {
        return propertyList_.insert(
                   std::make_pair(propertyName, propertyValue)
               ).second;
    }

    /// Set a new value to a property of the document.
    void updateProperty(const std::string& propertyName,
                        PropertyValue propertyValue)
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
    
    void copyPropertiesFromDocument(const Document& doc, bool override)
    {
        for(property_const_iterator it = doc.propertyBegin(); it!=doc.propertyEnd(); ++it)
        {
            if(!hasProperty(it->first))
            {
                property(it->first) = it->second;
            }
            else if(override)
            {
                property(it->first) = it->second;
            }
        }
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
        id_ = 0;
        propertyList_.clear();
    }

    bool isEmpty()
    {
        return ( propertyList_.size() == 0 );
    }

    void swap(Document& rhs)
    {
        using std::swap;
        swap(id_, rhs.id_);
        swap(propertyList_, rhs.propertyList_);
    }

private:
    /// document identifier in the collection
    docid_t id_;

    /// list of properties of the document
    std::map<std::string, PropertyValue> propertyList_;

    template<class DataIO>
    friend void DataIO_loadObject(DataIO& dio, Document& x);
    template<class DataIO>
    friend void DataIO_saveObject(DataIO& dio, const Document& x);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & id_;
        ar & propertyList_;
    }
};

inline void swap(Document& a, Document& b)
{
    a.swap(b);
}

template<class DataIO>
inline void DataIO_loadObject(DataIO& dio, Document& x)
{
    dio & x.id_;
    dio & x.propertyList_;
}
template<class DataIO>
inline void DataIO_saveObject(DataIO& dio, const Document& x)
{
    dio & x.id_;
    dio & x.propertyList_;
}

} // namespace sf1r

MAKE_FEBIRD_SERIALIZATION( sf1r::Document )

#endif // SF1V5_DOCUMENT_MANAGER_DOCUMENT_H

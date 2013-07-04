#ifndef SF1V5_DOCUMENT_MANAGER_DOCUMENT_H
#define SF1V5_DOCUMENT_MANAGER_DOCUMENT_H
/**
 * @file sf1r/document-manager/Document.h
 * @date Created <2009-10-20 16:12:04>
 * @date Updated <2009-10-22 03:04:08>
 */
#include <common/type_defs.h>
#include <common/PropertyValue.h>

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
    typedef PropertyValue::PropertyValueStrType doc_prop_value_strtype;
    typedef property_named_map::const_iterator property_const_iterator;
    typedef property_named_map::iterator property_iterator;

    Document() : id_(0)
    {
    }

    /// Construct a document with predefined document identifier
    explicit Document(docid_t id) : id_(id)
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
        return propertyMap_[propertyName];
    }
    const PropertyValue& property(const std::string& propertyName) const
    {
        static const PropertyValue not_found;
        property_const_iterator found = findProperty(propertyName);
        if (found != propertyEnd())
        {
            return found->second;
        }

        return not_found;
    }

    /// Insert a new property into the document.
    /// @return \c true if successful, \c false if already existed
    bool insertProperty(const std::string& propertyName,
                        const PropertyValue& propertyValue)
    {
        return propertyMap_.insert(
                   std::make_pair(propertyName, propertyValue)
               ).second;
    }

    /// Set a new value to a property of the document.
    void updateProperty(const std::string& propertyName,
                        const PropertyValue& propertyValue)
    {
        propertyMap_[propertyName] = propertyValue;
    }

    /// Remove a property from the document. Return false if the property
    /// can not be removed.
    /// @return true if successful, false otherwise
    bool eraseProperty(const std::string& propertyName)
    {
        return propertyMap_.erase(propertyName);
    }

    property_iterator findProperty(const std::string& propertyName)
    {
        return propertyMap_.find(propertyName);
    }
    property_const_iterator findProperty(const std::string& propertyName) const
    {
        return propertyMap_.find(propertyName);
    }

    bool hasProperty(const std::string& pname) const
    {
        property_const_iterator it = findProperty(pname);
        return it != propertyEnd();
    }

    /// may throw bad cast exception if type not match
    template <class T>
    bool getProperty(const std::string& pname, T& value) const
    {
        property_const_iterator it = findProperty(pname);
        if (it == propertyEnd()) return false;
        value = it->second.get<T>();
        return true;
    }

    bool getString(const std::string& pname, std::string& value) const
    {
        //izenelib::util::UString ustr;
        //if(!getProperty(pname, ustr)) return false;
        //ustr.convertString(value, izenelib::util::UString::UTF_8);

        //
        // the newest code has changed document property internal value to std::string 
        if(!getProperty(pname, value)) return false;
        return true;
    }

    doc_prop_value_strtype& getString(const std::string& pname)
    {
        return property(pname).get<doc_prop_value_strtype>();
    }

    const doc_prop_value_strtype& getString(const std::string& pname) const
    {
        return property(pname).get<doc_prop_value_strtype>();
    }


    property_iterator propertyBegin()
    {
        return propertyMap_.begin();
    }
    property_iterator propertyEnd()
    {
        return propertyMap_.end();
    }
    property_const_iterator propertyBegin() const
    {
        return propertyMap_.begin();
    }
    property_const_iterator propertyEnd() const
    {
        return propertyMap_.end();
    }

    void clearProperties()
    {
        propertyMap_.clear();
    }

    void clear()
    {
        id_ = 0;
        clearProperties();
    }

    void clearExceptDOCID()
    {
        izenelib::util::UString docid;
        getProperty("DOCID", docid);
        clear();
        property("DOCID") = docid;
    }

    bool isEmpty() const
    {
        return propertyMap_.empty();
    }

    void swap(Document& rhs)
    {
        std::swap(id_, rhs.id_);
        propertyMap_.swap(rhs.propertyMap_);
    }

    size_t getPropertySize() const
    {
        return propertyMap_.size();
    }

    bool copyPropertiesFromDocument(const Document& doc, bool override = true)
    {
        bool modified = false;
        for (property_const_iterator it = doc.propertyBegin(); it != doc.propertyEnd(); ++it)
        {
            if (!hasProperty(it->first) || override)
            {
                property(it->first) = it->second;
                modified = true;
            }
        }
        return modified;
    }
    void merge(const Document& doc)
    {
        copyPropertiesFromDocument(doc, true);
    }

    void diff(const Document& doc, Document& diff_doc) const
    {
        for(property_const_iterator it=propertyBegin();it!=propertyEnd();++it)
        {
            const std::string& key=it->first;
            property_const_iterator ait=doc.findProperty(key);
            bool dd=true;
            if(key=="DOCID"||ait==doc.propertyEnd())
            {
                dd=false;
            }
            else
            {
                if(it->second!=ait->second)
                {
                    dd=false;
                }
            }
            if(!dd)
            {
                diff_doc.property(key)=it->second;
            }
        }
    }
    void diff(const Document& doc)
    {
        std::vector<std::string> erase_vec;
        for(property_const_iterator it=propertyBegin();it!=propertyEnd();++it)
        {
            const std::string& key=it->first;
            property_const_iterator ait=doc.findProperty(key);
            bool dd=true;
            if(key=="DOCID"||ait==doc.propertyEnd())
            {
                dd=false;
            }
            else
            {
                if(it->second!=ait->second)
                {
                    dd=false;
                }
            }
            if(dd)
            {
                erase_vec.push_back(key);
            }
        }
        for(uint32_t i=0;i<erase_vec.size();i++)
        {
            eraseProperty(erase_vec[i]);
        }
    }

private:
    /// document identifier in the collection
    docid_t id_;

    /// list of properties of the document
    property_named_map propertyMap_;

    template<class DataIO>
    friend void DataIO_loadObject(DataIO& dio, Document& x);
    template<class DataIO>
    friend void DataIO_saveObject(DataIO& dio, const Document& x);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & id_;
        ar & propertyMap_;
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
    dio & x.propertyMap_;
}
template<class DataIO>
inline void DataIO_saveObject(DataIO& dio, const Document& x)
{
    dio & x.id_;
    dio & x.propertyMap_;
}

} // namespace sf1r

MAKE_FEBIRD_SERIALIZATION( sf1r::Document )

#endif // SF1V5_DOCUMENT_MANAGER_DOCUMENT_H

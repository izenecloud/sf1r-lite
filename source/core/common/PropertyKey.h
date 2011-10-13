#ifndef SF1V5_DOCUMENT_MANAGER_PROPERTY_KEY_H
#define SF1V5_DOCUMENT_MANAGER_PROPERTY_KEY_H
/**
 * @file sf1r/document-manager/PropertyKey.h
 * @date Created <2009-10-20 17:30:23>
 * @date Updated <2009-10-21 17:47:54>
 */
#include "type_defs.h"

#include <util/izene_serialization.h>

namespace sf1r {

class PropertyKey {
public:
    docid_t docId;
    propertyid_t propertyId;
    bool isDeleted;

    PropertyKey()
    : docId(0)
    , propertyId(0)
    , isDeleted(false)
    {
    }

    PropertyKey(docid_t docId,
                propertyid_t pId)
    : docId(docId)
    , propertyId(pId)
    , isDeleted(false)
    {
    }

    ~PropertyKey() {}

    PropertyKey& operator=(const PropertyKey& okey)
    {
        docId = okey.docId;
        propertyId = okey.propertyId;
        isDeleted = okey.isDeleted;

        return *this;
    }

    void set(docid_t docId,
             propertyid_t pId)
    {
        docId = docId;
        propertyId = pId;
        isDeleted = false;
    }

    void clear() {
        docId = 0;
        propertyId = 0;
        isDeleted = false;
    }
    

    bool markDelete(docid_t docId , propertyid_t pId)
    {
        docId = docId;
        propertyId = pId;
        isDeleted = true;

        return true;
    }


    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& docId;
        ar& propertyId;
        ar& isDeleted;
    }
    DATA_IO_LOAD_SAVE(PropertyKey, &docId&propertyId&isDeleted);

    int compare(const PropertyKey& other) const
    {
        if (docId != other.docId) {
            return (docId - other.docId);
        } else {
            return (propertyId - other.propertyId);
        }
    }
};

inline bool operator==(const PropertyKey& a,
                       const PropertyKey& b)
{
    return a.docId == b.docId
        && a.propertyId == b.propertyId;
}

inline bool operator!=(const PropertyKey& a,
                       const PropertyKey& b)
{
    return !(a == b);
}

} // namespace sf1r

#endif // SF1V5_DOCUMENT_MANAGER_PROPERTY_KEY_H

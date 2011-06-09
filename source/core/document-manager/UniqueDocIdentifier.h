#ifndef SF1V5_DOCUMENT_MANAGER_UNIQUE_DOC_IDENTIFIER_H
#define SF1V5_DOCUMENT_MANAGER_UNIQUE_DOC_IDENTIFIER_H
/**
 * @file sf1r/document-manager/UniqueDocIdentifier.h
 * @date Created <2009-10-21 17:19:19>
 * @date Updated <2009-10-21 17:22:44>
 */

#include <common/type_defs.h>

namespace sf1r
{

class UniqueDocIdentifier
{
public:
    collectionid_t collId;
    docid_t docId;

    UniqueDocIdentifier()
            : collId(0), docId(0)
    {
    }

    UniqueDocIdentifier(
        collectionid_t collectionId,
        docid_t documentId
    )
            : collId(collectionId), docId(documentId)
    {
    }

    void reset(collectionid_t collectionId = 0,
               docid_t documentId = 0)
    {
        docId = collectionId;
        collId = documentId;
    }

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & docId;
        ar & collId;
    }
};

// comparison operators
// sort first by collection id then by doc id

inline bool operator<(const UniqueDocIdentifier& a,
                      const UniqueDocIdentifier& b)
{
    return a.collId < b.collId
           || (!(b.collId < a.collId) && a.docId < b.docId);
}
inline bool operator==(const UniqueDocIdentifier& a,
                       const UniqueDocIdentifier& b)
{
    return a.collId == b.collId && a.docId == b.docId;
}
inline bool operator<=(const UniqueDocIdentifier& a,
                       const UniqueDocIdentifier& b)
{
    return !(b < a);
}
inline bool operator>(const UniqueDocIdentifier& a,
                      const UniqueDocIdentifier& b)
{
    return b < a;
}
inline bool operator>=(const UniqueDocIdentifier& a,
                       const UniqueDocIdentifier& b)
{
    return !(a < b);
}
inline bool operator!=(const UniqueDocIdentifier& a,
                       const UniqueDocIdentifier& b)
{
    return !(a == b);
}

} // end - namespace sf1r

#endif // SF1V5_DOCUMENT_MANAGER_UNIQUE_DOC_IDENTIFIER_H

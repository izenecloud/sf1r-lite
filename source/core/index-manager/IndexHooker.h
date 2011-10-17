#ifndef SF1V5_INDEX_MANAGER__INDEXHOOKER_H
#define SF1V5_INDEX_MANAGER__INDEXHOOKER_H

#include <common/type_defs.h>
#include <document-manager/Document.h>

namespace sf1r
{

class IndexHooker
{
public:
    virtual ~IndexHooker();
    virtual bool HookInsert(Document& doc);
    virtual bool HookUpdate(docid_t oldid, Document& doc, bool r_type);
    virtual bool HookDelete(docid_t docid);
    virtual bool Finish();
};


 
}

#endif

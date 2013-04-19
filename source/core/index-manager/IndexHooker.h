#ifndef SF1V5_INDEX_MANAGER__INDEXHOOKER_H
#define SF1V5_INDEX_MANAGER__INDEXHOOKER_H

#include <common/type_defs.h>
#include <document-manager/Document.h>

namespace sf1r
{

class IndexHooker
{
public:
    virtual ~IndexHooker() {}
    virtual bool HookInsert(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp)
    {return false;}
    virtual bool HookUpdate(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp)
    {return false;}
//     virtual bool HookInsert(Document& doc) {return false;}
//     virtual bool HookUpdate(docid_t oldid, Document& doc, bool r_type) {return false;}
    virtual bool HookDelete(docid_t docid, time_t timestamp) {return false;}
    virtual bool FinishHook(time_t timestamp) {return false;}
};

}

#endif

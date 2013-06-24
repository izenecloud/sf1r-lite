#ifndef SF1V5_INDEX_MANAGER_IINCSUPPORTEDINDEX_H
#define SF1V5_INDEX_MANAGER_IINCSUPPORTEDINDEX_H

namespace sf1r
{
class Document;
// Define the interface for the index which will support increment build.
// This kind of index will be updated while iterating the SCD files or 
// some update/insert/delete api coming.
class IIncSupportedIndex
{
public:
    virtual ~IIncSupportedIndex() {}
    virtual void flush(bool force) = 0;
    virtual void optimize(bool wait) = 0;

    virtual void preBuildFromSCD(size_t total_filesize) = 0;
    virtual void postBuildFromSCD(time_t timestamp) = 0;

    virtual void preMining() = 0;
    virtual void postMining() = 0;

    virtual void finishIndex() = 0;

    virtual void finishRebuild() = 0;

    virtual void preProcessForAPI() = 0;
    virtual void postProcessForAPI() = 0;

    virtual bool insertDocument(const Document& doc, time_t timestamp) = 0;
    virtual bool updateDocument(const Document& olddoc, const Document& newdoc,
        int updateType, time_t timestamp) = 0;
    virtual void removeDocument(docid_t docid, time_t timestamp) = 0;

};

}

#endif

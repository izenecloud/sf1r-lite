/**
 * @file ZambeziIndexManager.h
 * @brief build zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_INDEX_MANAGER_H
#define SF1R_ZAMBEZI_INDEX_MANAGER_H

#include "IIncSupportedIndex.h"
#include "zambezi-manager/ZambeziManager.h"
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <process/common/XmlConfigParser.h>

namespace sf1r
{
class ZambeziConfig;
class ZambeziTokenizer;

class ZambeziIndexManager : public IIncSupportedIndex
{
public:
    ZambeziIndexManager(
        const ZambeziConfig& config,
        const std::vector<std::string>& properties,
        std::map<std::string, AttrIndex>& property_index_map);
    ~ZambeziIndexManager();

    virtual bool isRealTime() { return false; }
    virtual void flush(bool force) {}
    virtual void optimize(bool wait) {}

    virtual void preBuildFromSCD(std::size_t total_filesize) {}
    virtual void postBuildFromSCD(time_t timestamp);

    virtual void preMining() {}
    virtual void postMining() {}

    virtual void finishIndex() {}

    virtual void finishRebuild() {}

    virtual void preProcessForAPI() {}
    virtual void postProcessForAPI() {}

    virtual bool insertDocument(
        const Document& doc,
        time_t timestamp);

    virtual bool updateDocument(
        const Document& olddoc,
        const Document& old_rtype_doc,
        const Document& newdoc,
        int updateType,
        time_t timestamp);

    virtual void removeDocument(docid_t docid, time_t timestamp) {}

    void buildTokenizeDic();
private:
    bool buildDocument_(const Document& doc);

    bool buildDocument_Normal_(const Document& doc, const std::string& property);

    bool buildDocument_Combined_(const Document& doc, const std::string& property);

    bool buildDocument_Attr_(const Document& doc, const std::string& property);

    bool insertDocIndex_(
        const docid_t docId, 
        const std::string property,
        const std::vector<std::pair<std::string, int> >& tokenScoreList);

private:
    std::string system_resource_path_;

    const ZambeziConfig& config_;
    const std::vector<std::string>& properties_;

    std::map<std::string, AttrIndex>& property_index_map_;
};


} // namespace sf1r

#endif // SF1R_ZAMBEZI_INDEX_MANAGER_H

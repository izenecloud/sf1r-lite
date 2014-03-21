#ifndef PROCESS_CONTROLLERS_DOCUMENTS_GET_HANDLER_H
#define PROCESS_CONTROLLERS_DOCUMENTS_GET_HANDLER_H
/**
 * @file process/controllers/DocumentsGetHandler.h
 * @author Ian Yang
 * @date Created <2010-06-02 16:05:36>
 * @brief Handle getting documents by id and doc_id
 */
#include <common/inttypes.h> // docid_t

#include "CollectionHandler.h"

#include <util/driver/Request.h>
#include <util/driver/Response.h>

#include <query-manager/ActionItem.h>

#include <vector>
#include <string>

namespace sf1r
{

using namespace izenelib::driver;

class DocumentsGetHandler
{
    static const std::size_t kMaxSimilarDocumentCount;

public:
    DocumentsGetHandler(
        ::izenelib::driver::Request& request,
        ::izenelib::driver::Response& response,
        const CollectionHandler& collectionHandler
    );

    /// @internal
    /// @brief Handle documents/get
    void get();

    bool parseSelect();
    bool parseSearchSession();
    bool parsePageInfo();
    bool parseDocumentId(const Value& inputId);
    bool getIdListFromConditions();

    bool doGet();

    void aclFilter();

private:
    ::izenelib::driver::Request& request_;
    ::izenelib::driver::Response& response_;
    IndexSearchService* indexSearchService_;
    MiningSearchService* miningSearchService_;
    const IndexBundleSchema& indexSchema_;
    const MiningSchema& miningSchema_;
    const ZambeziConfig& zambeziConfig_;

    GetDocumentsByIdsActionItem actionItem_;

    /// for parseDocumentId
    sf1r::wdocid_t internalId_;

    /// for parsePageInfo
    std::size_t offset_;
    std::size_t limit_;
};

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_DOCUMENTS_GET_HANDLER_H

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

    /// @internal
    /// @brief Handle documents/similar_to
    void similar_to();

    /// @internal
    /// @brief Handle documents/duplicate_with
    void duplicate_with();

    /// @internal
    /// @brief Handle documents/similar_to_image
    void similar_to_image();

    bool parseSelect();
    bool parseSearchSession();
    bool parsePageInfo();
    bool parseDocumentId(const Value& inputId);
    bool getIdListFromConditions();

    void doGet();

    void aclFilter();

private:
    ::izenelib::driver::Request& request_;
    ::izenelib::driver::Response& response_;
    IndexSearchService* indexSearchService_;
    MiningSearchService* miningSearchService_;
    IndexBundleSchema indexSchema_;

    GetDocumentsByIdsActionItem actionItem_;

    /// for parseDocumentId
    sf1r::docid_t internalId_;

    /// for parsePageInfo
    std::size_t offset_;
    std::size_t limit_;
};

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_DOCUMENTS_GET_HANDLER_H

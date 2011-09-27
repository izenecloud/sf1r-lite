#ifndef CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_SEMANTIC_KERNEL_H
#define CORE_MINING_MANAGER_SIMILARITY_DETECTION_SUBMANAGER_SEMANTIC_KERNEL_H

#include <la-manager/AnalysisInformation.h>
#include <query-manager/SearchKeywordOperation.h>
#include "PrunedSortedForwardIndex.h"
#include <ir/id_manager/IDManager.h>
#include <common/ResultType.h>

#include <util/ustring/UString.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

#include <fstream>

namespace sf1r{

using izenelib::ir::idmanager::IDManager;

class DocumentManager;
class IndexManager;
class LAManager;
class SearchManager;
class SemanticKernel : public boost::noncopyable
{
public:
    SemanticKernel(
        const std::string& workingDir,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager,
        boost::shared_ptr<LAManager> laManager,
        boost::shared_ptr<IDManager> idManager,
        boost::shared_ptr<SearchManager> searchManager
    );

    ~SemanticKernel();

    void setSchema(
        const std::vector<uint32_t>& property_ids,
        const std::vector<std::string>& properties,
        const std::string& semanticField,
        const std::string& domainField
    );

    void doSearch();

private:
    bool buildQuery_(SearchKeywordOperation&action, izenelib::util::UString& queryUStr);

    void flushSim_(uint32_t did, float weight, std::fstream& out);
private:

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;
    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<SearchManager> searchManager_;

    std::string semanticField_;
    std::string domainField_;
    std::vector<std::string> searchPropertyList_;

    AnalysisInfo analysisInfo_;
    PrunedSortedForwardIndex* forwardIndexBuilder_;

    std::string allpairPath_;
};

}
#endif


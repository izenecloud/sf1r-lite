#ifndef PROCESS_CONTROLLERS_DOCUMENTS_CONTROLLER_H
#define PROCESS_CONTROLLERS_DOCUMENTS_CONTROLLER_H
/**
 * @file process/controllers/DocumentsController.h
 * @author Ian Yang
 * @date Created <2010-06-02 09:58:52>
 */
#include "Sf1Controller.h"

#include <string>
#include <vector>

namespace sf1r
{
class IndexTaskService;
class IndexSearchService;
class MiningSearchService;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b documents
 *
 * Create, update, destroy or fetch documents.
 */
class DocumentsController : public Sf1Controller
{
    /// @brief default count in page.
    static const std::size_t kDefaultPageCount;

public:
    DocumentsController();

    /// @brief alias for search
    void index()
    {
        search();
    }

    void search();
    void get();
    void similar_to();
    void duplicate_with();
    void similar_to_image();
    void create();
    void update();
    void destroy();
    void get_topic();
    void get_topic_with_sim();

    void log_group_label();
    void get_freq_group_labels();
    void set_top_group_label();

    void visit();
    void get_summarization();
    void get_doc_count();
    void get_key_count();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    bool requireDOCID();
    bool setLimit();

    bool requireKeywords(std::string& keywords);
    bool requireGroupProperty(std::string& groupProperty);

    typedef std::vector<std::string> GroupPath;
    bool requireGroupLabel(GroupPath& groupPath);
    bool requireGroupLabelVec(std::vector<GroupPath>& groupPathVec);

    IndexSearchService* indexSearchService_;
    IndexTaskService* indexTaskService_;
    MiningSearchService* miningSearchService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_DOCUMENTS_CONTROLLER_H

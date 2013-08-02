/**
 * @file MultiThreadMiningTaskBuilder.h
 * @brief execute mining task in multi-threads.
 */

#ifndef SF1R_MULTI_THREAD_MINING_TASK_BUILDER_H
#define SF1R_MULTI_THREAD_MINING_TASK_BUILDER_H

#include <common/inttypes.h>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class DocumentManager;
class MiningTask;

class MultiThreadMiningTaskBuilder
{
public:
    explicit MultiThreadMiningTaskBuilder(
        boost::shared_ptr<DocumentManager>& documentManager,
        std::size_t threadNum);

    ~MultiThreadMiningTaskBuilder();

    void addTask(MiningTask* task);

    bool buildCollection();

private:
    void buildDocs_(
        docid_t startDocId,
        docid_t endDocId,
        std::size_t threadId);

private:
    boost::shared_ptr<DocumentManager> documentManager_;

    const std::size_t threadNum_;

    std::vector<MiningTask*> taskList_;

    std::vector<bool> taskFlag_; // true for build, false for not build
};

} // namespace sf1r

#endif // SF1R_MULTI_THREAD_MINING_TASK_BUILDER_H

///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.28
///
#ifndef SOURCE_CORE_MINING_MANAGER_MININGTASKBUILDER_H_
#define SOURCE_CORE_MINING_MANAGER_MININGTASKBUILDER_H_

#include <boost/shared_ptr.hpp>
#include "MiningTask.h"

namespace sf1r
{
class DocumentManager;

class MiningTaskBuilder
{
public:
    explicit MiningTaskBuilder(boost::shared_ptr<DocumentManager> document_manager);
    ~MiningTaskBuilder();

    bool buildCollection();
    void addTask(MiningTask*);

private:
    std::vector<MiningTask*> taskList_;
    boost::shared_ptr<DocumentManager> document_manager_;

};
}

#endif  // SOURCE_CORE_MINING_MANAGER_MININGTASKBUILDER_H_

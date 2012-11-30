///
/// @file MiningTask.h
/// @author hongliang.zhao@b5m.com
/// @date Created 2012.11.28
///
#ifndef MING_TASK_BUILDER_H
#define MING_TASK_BUILDER_H

#include "MiningTask.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

using namespace std;
namespace sf1r
{
class DocumentManager;

class MiningTaskBuilder
{
public:
	MiningTaskBuilder(boost::shared_ptr<DocumentManager> document_manager);
	~MiningTaskBuilder() {} ;

	bool buildCollection();
	void addTask(MiningTask*);

private:

	std::vector<MiningTask*> taskList_;
	boost::shared_ptr<DocumentManager> document_manager_;

};
}
#endif
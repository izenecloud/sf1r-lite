#ifndef SF1R_RECOMMEND_REBUILDER_H
#define SF1R_RECOMMEND_REBUIDLER_H

#include <common/file-monitor/FileMonitor.h>
#include <common/file-monitor/FileEventHandler.h>
#include <boost/function.hpp>

namespace sf1r
{
namespace Recommend
{
    typedef boost::function<void()> RebuildHanlder;
}
class Rebuilder : public FileEventHandler
{
public:
    static Rebuilder* get();

    Rebuilder();

    virtual bool handle(const std::string& fileName, uint32_t mask);

    void start(const std::string& dictDir);

    void addRebuildHanlder(Recommend::RebuildHanlder* handler);
private:
    FileMonitor monitor_;
    std::string dictDir_;
    bool isStart_;
    std::vector<Recommend::RebuildHanlder*> handlers_;
};

} // namespace sf1r

#endif 

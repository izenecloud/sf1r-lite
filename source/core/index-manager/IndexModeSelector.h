/**
 * @author Jia Guo
 * @brief Modify index manager mode(default or realtime) at runtime automatically.
 */

#ifndef SF1R_INDEXMANAGER_INDEXMODESELECTOR_H_
#define SF1R_INDEXMANAGER_INDEXMODESELECTOR_H_

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class IndexManager;

class IndexModeSelector
{
public:
    IndexModeSelector(const boost::shared_ptr<IndexManager>& index_manager, double threshold, long max_realtime_msize);

    void TrySetIndexMode(long scd_file_size);

    void TryCommit();

    void ForceCommit();

private:
    boost::shared_ptr<IndexManager> index_manager_;
    double threshold_;
    long max_realtime_msize_;
};

}

#endif

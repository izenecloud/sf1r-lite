#ifndef IMG_DUP_PROCESS_H_
#define IMG_DUP_PROCESS_H_

#include "common/OnSignal.h"
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class ImgDupProcess
{
public:
    ImgDupProcess();
    ~ImgDupProcess();

    bool init(const std::string& cfgFile);
    /// @brief start all
    void start();

    /// @brief join all
    void join();

    /// @brief interrupt
    void stop();
};


#endif /* IMG_DUP_PROCESS_H_ */

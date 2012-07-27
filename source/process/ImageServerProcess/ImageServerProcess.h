#ifndef COLOR_SERVER_PROCESS_H_
#define COLOR_SERVER_PROCESS_H_

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class RpcImageServer;

class ImageServerProcess
{
public:
    ImageServerProcess();

    ~ImageServerProcess();

    bool init(const std::string& cfgFile);

    /// @brief start all
    void start();

    /// @brief join all
    void join();

    /// @brief interrupt
    void stop();

private:
    bool initRpcImageServer();

private:
    boost::scoped_ptr<RpcImageServer> rpcImageServer_;
};

}

#endif /* COLOR_SERVER_PROCESS_H_ */

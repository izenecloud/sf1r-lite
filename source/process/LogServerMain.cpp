#include <ProcessOptions.h>
#include <log-server/LogServerProcess.h>

#include <unistd.h>

using namespace sf1r;

__pid_t getPid();

int main(int argc, char* argv[])
{
    try
    {
        ProcessOptions po;
        std::vector<std::string> args(argv + 1, argv + argc);

        if (po.setLogServerProcessArgs(argv[0], args))
        {
            __pid_t pid = getPid();
            LOG(INFO) << "\tLog Server Process : pid=" << pid;

            LogServerProcess logServerProcess;

            if (logServerProcess.init(po.getConfigFile()))
            {
                logServerProcess.start();
                logServerProcess.join();
            }
        }
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    exit(1);
}

__pid_t getPid()
{
    return ::getpid();
}

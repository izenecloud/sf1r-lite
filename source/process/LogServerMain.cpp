#include <ProcessOptions.h>
#include <log-server/LogServer.h>


using namespace sf1r;

int main(int argc, char* argv[])
{
    try
    {
        ProcessOptions po;
        std::vector<std::string> args(argv + 1, argv + argc);
        if (po.setLogServerProcessArgs(argv[0], args))
        {
            LogServer logServer;
            logServer.start();
        }
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    exit(1);
}

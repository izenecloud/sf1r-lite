#include <ProcessOptions.h>
#include <log-server/LogServer.h>


using namespace sf1r;

int main(int argc, char* argv[])
{
    try
    {
        ProcessOptions po;
        std::vector<std::string> args(argv + 1, argv + argc);


        LogServer logServer;
        logServer.start();
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

#include "CobraProcess.h"
#include "common/ProcessOptions.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>

using namespace std;
namespace bf = boost::filesystem;

void savePid(string pidFile);

int main(int argc, char * argv[])
{
    try
    {
        ProcessOptions po;
        string filePath;
        std::vector<std::string> args(argv + 1, argv + argc);
        if (po.setCobraProcessArgs(args))
        {
            filePath = po.getConfigFileDirectory() ;
            string pidFile = po.getPidFile();
            if (pidFile.empty())
            {
                std::cerr << "Pid file is not saved" << std::endl;
            }
            else
            {
                ::savePid(pidFile);
            }
            CobraProcess cobraProcess;
            if (!cobraProcess.initialize(filePath))
                return 1;
            return cobraProcess.run();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exit because of exception: "
                  << e.what() << std::endl;
    }

    return 1;
}

void savePid(string pidFile)
{
    try
    {
        long pid = static_cast<long>(::getpid());
        bf::path file(pidFile);
        bf::path dir = file.parent_path();
        if (!dir.empty())
        {
            create_directories(dir);
        }

        bf::ofstream ofs(file);
        ofs << pid;

        if (ofs)
        {
            ofs.close();
            return;
        }
    }
    catch (const std::exception& ex)
    {
        // ignore
    }

    std::cerr << "Cannot save pidFile to " << pidFile << std::endl;
    ::exit(1);
    return;
}

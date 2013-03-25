#ifndef LOG_SERVER_SKETCH_MANAGER_
#define LOG_SERVER_SKETCH_MANAGER_

#include <iostream>
#include <string>
#include <fstream>
//#include <madoka.h>

using namespace std;

namespace sf1r{
class LogServerSketchManager
{
public:
    LogServerSketchManager(std::string wd)
    {
        workdir_ = wd;
    }

    bool open()
    {
        return true;
    }

    bool close()
    {
        return true;
    }
private:
    std::string workdir_;

};

}//end of namespace sf1r

#endif

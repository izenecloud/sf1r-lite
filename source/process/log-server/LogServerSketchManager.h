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

    void close()
    {

    }

    bool injectUserQuery(const std::string & query,
            const std::string & collection,
            const uint32_t & hitnum,
            const uint32_t & page_start,
            const uint32_t & page_count,
            const std::string & duration,
            const std::string & timestamp)
    {
        if(!checkSketch(collection))
            return false;
        //write file
        //write sketch
        return true;
    }

    bool getFreqUserQueries(const std::string & collection,
            const std::string & begin_time,
            const std::string & end_time,
            std::list< std::map<std::string, std::string> > & results)
    {
        if(!checkSketch(collection))
            return false;
        //a temp sketch should be built
        //read result list from cache
        //read result list from file
        return true;
    }

private:
    bool checkSketch(const std::string& collection)
    {
/*
        if(collectionSketchMap_.find(collection) == collectionSketchMap_.end())
        {
            initSketchFile(collection);
        }
*/
        return 0;
    }

    bool initSketchFile(const std::string& collection)
    {
        return true;
    }
    std::string workdir_;


};

}//end of namespace sf1r

#endif

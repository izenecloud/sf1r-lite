#ifndef COBRAPROCESS_H_
#define COBRAPROCESS_H_

#include <configuration-manager/CobraConfig.h>

#include <string>

class CobraProcess
{
public:
    CobraProcess() {}
    int run();
    bool initialize(const std::string& configFilePath);
private:
    CobraConfig config_;	
};

#endif /*COBRAPROCESS_H_*/

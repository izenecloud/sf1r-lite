#ifndef COBRAPROCESS_H_
#define COBRAPROCESS_H_

#include <string>

class CobraProcess
{
public:
    CobraProcess() {}
    int run();
    bool initialize(const std::string& configFilePath);
private:
};

#endif /*COBRAPROCESS_H_*/

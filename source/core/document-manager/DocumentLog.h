#ifndef _DOCUMENT_LOG_H_
#define _DOCUMENT_LOG_H_

#include <string>

namespace sf1r
{

class DocumentLog
{
public:
    enum LogType
    {
        TYPE_INFO = 0,
        TYPE_DEBUG,
        TYPE_WARN,
        TYPE_ERROR,
        TYPE_CRIT,
        TYPE_PERF,
    };

public:
    static bool print(LogType type, const std::string& module, const std::string& message,
                      std::ostream& os = std::cerr);

private:
    static const std::string logTypeStrings_[6];
};


} // end - namespace sf1r

#endif // _DOCUMENT_LOG_H_

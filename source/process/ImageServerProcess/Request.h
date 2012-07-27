#ifndef __REQUEST__H__
#define __REQUEST__H__

#include <boost/shared_ptr.hpp>
#define OP_ADD 1
#define OP_DEL 0
#pragma pack(push)
#pragma pack(1)

struct Request
{
    std::string filePath;
    std::string newFilePath;
    //int result;
};
#pragma pack(pop)
typedef boost::shared_ptr<Request> MSG;
#endif

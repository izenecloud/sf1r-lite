#ifndef __REQUEST__H__
#define __REQUEST__H__

#include <boost/shared_ptr.hpp>
#define OP_ADD 1
#define OP_DEL 0

enum FileType
{
    FILE_SCD,
    FILE_LIST
};
#pragma pack(push)
#pragma pack(1)


struct Request
{
    std::string filePath;
    int  filetype;
};
#pragma pack(pop)
typedef boost::shared_ptr<Request> MSG;
#endif

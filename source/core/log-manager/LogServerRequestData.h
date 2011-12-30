#ifndef LOG_SERVER_REQUEST_DATA_H_
#define LOG_SERVER_REQUEST_DATA_H_

#include <string>
#include <vector>
#include <sstream>

#include <3rdparty/msgpack/msgpack.hpp>

namespace sf1r
{

struct LogServerRequestData
{
};

/// xxx modify on practical use
struct UUID2DocidList : public LogServerRequestData
{
    typedef std::vector<uint32_t> DocidListType;

    std::string uuid_;
    DocidListType docidList_;

    std::string toString() const
    {
        std::ostringstream oss;
        oss << uuid_ << " ->";

        for (size_t i = 0; i < docidList_.size(); i++)
        {
            oss << " " << docidList_[i];
        }

        return oss.str();
    }

    MSGPACK_DEFINE(uuid_, docidList_)
};

}

#endif /* LOG_SERVER_REQUEST_DATA_H_ */

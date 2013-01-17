#ifndef NODE_REQUEST_LOG_H_
#define NODE_REQUEST_LOG_H_

#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <memory.h>
#include <vector>
#include <set>

namespace sf1r
{

enum ReqLogType
{
    Req_None =0,
    Req_Index = 1,
    Req_CreateDoc = 2,
};

#pragma pack(1)
struct ReqLogHead
{
    uint32_t inc_id;
    uint32_t reqtype;
    uint32_t req_data_offset;
    uint32_t req_data_len;
    uint32_t req_data_crc;
    char reqtime[16];
};
#pragma pack()

struct CommonReqData
{
    uint32_t inc_id;
    uint32_t reqtype;
    std::string req_json_data;
    MSGPACK_DEFINE(inc_id, reqtype, req_json_data);
};

// note: head data will not be packed to msgpack.
// head data will store separately for index.
struct IndexReqLog
{
    IndexReqLog()
    {
        common_data.reqtype = Req_Index;
    }
    CommonReqData common_data;
    std::vector<std::string> scd_list;
    MSGPACK_DEFINE(common_data, scd_list);
};

struct CreateDocReqLog
{
    CreateDocReqLog()
    {
        common_data.reqtype = Req_CreateDoc;
    }
    CommonReqData common_data;
    MSGPACK_DEFINE(common_data);
};

static const uint32_t _crc32tab[] = {
    0x0,0x77073096,0xee0e612c,0x990951ba,0x76dc419,0x706af48f,0xe963a535,0x9e6495a3,0xedb8832,0x79dcb8a4,
    0xe0d5e91e,0x97d2d988,0x9b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,
    0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,
    0xfa0f3d63,0x8d080df5,0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
    0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,0x26d930ac,0x51de003a,
    0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,
    0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,0x76dc4190,0x1db7106,0x98d220bc,0xefd5102a,0x71b18589,0x6b6b51f,
    0x9fbfe4a5,0xe8b8d433,0x7807c9a2,0xf00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x86d3d2d,0x91646c97,0xe6635c01,
    0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,0x65b0d9c6,0x12b7e950,
    0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,
    0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,
    0xaa0a4c5f,0xdd0d7cc9,0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
    0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,0xedb88320,0x9abfb3b6,
    0x3b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x4db2615,0x73dc1683,0xe3630b12,0x94643b84,0xd6d6a3e,0x7a6a5aa8,
    0xe40ecf0b,0x9309ff9d,0xa00ae27,0x7d079eb1,0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,
    0x196c3671,0x6e6b06e7,0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
    0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,0xd80d2bda,0xaf0a1b4c,
    0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,
    0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,
    0x2cd99e8b,0x5bdeae1d,0x9b64c2b0,0xec63f226,0x756aa39c,0x26d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x5005713,
    0x95bf4a82,0xe2b87a14,0x7bb12bae,0xcb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0xbdbdf21,0x86d3d2d4,0xf1d4e242,
    0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,
    0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,
    0x4969474d,0x3e6e77db,0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
    0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,0xb3667a2e,0xc4614ab8,
    0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d};


// each log file will store 1000 write request.
// 1 - 999  saved in 0.req.log
// 1000 - 1999 saved in 1.req.log and so on.
// head.req.log store the offset, length and some other info for each write request.
class ReqLogMgr
{
public:
    ReqLogMgr()
        :inc_id_(1),
        last_writed_id_(0)
    {
    }

    static void initWriteRequestSet()
    {
        write_req_set_.insert("documents_create");
    }

    static inline bool isWriteRequest(const std::string& controller, const std::string& action)
    {
        if (write_req_set_.find(controller + "_" + action) != write_req_set_.end())
            return true;
        return false;
    }

    std::string getRequestLogPath()
    {
        return base_path_;
    }

    void init(const std::string& basepath)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        inc_id_ = 1;
        last_writed_id_ = 0;
        base_path_ = basepath;
        head_log_path_ = basepath + "/head.req.log";
        loadLastData();
    }

    bool prepareReqLog(CommonReqData& prepared_reqdata, bool isprimary)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        if (!prepared_req_.empty())
        {
            std::cout << "already has a prepared Request, only one write request allowed." << endl;
            return false;
        }
        if (isprimary)
            prepared_reqdata.inc_id = inc_id_++;
        else
        {
            inc_id_ = prepared_reqdata.inc_id + 1;
        }
        prepared_req_.push_back(prepared_reqdata);
        printf("prepare request success, isprimary %d, (id:%u, type:%u) ", isprimary,
            prepared_reqdata.inc_id, prepared_reqdata.reqtype);
        std::cout << endl;
        return true;
    }

    bool getPreparedReqLog(CommonReqData& reqdata)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        if (prepared_req_.empty())
            return false;
        reqdata = prepared_req_.back();
        return true;
    }

    void delPreparedReqLog()
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        prepared_req_.clear();
    }

    template <typename TypedReqLog> bool appendTypedReqLog(const TypedReqLog& reqdata)
    {
        msgpack::sbuffer buf;
        msgpack::pack(buf, reqdata);
        return appendReqData(std::string(buf.data(), buf.size()));
    }
    template <typename TypedReqLog> static void packReqLogData(const TypedReqLog& reqdata, std::string& packed_data)
    {
        msgpack::sbuffer buf;
        msgpack::pack(buf, reqdata);
        packed_data.assign(buf.data(), buf.size());
    }

    template <typename TypedReqLog> static bool unpackReqLogData(const std::string& packed_data, TypedReqLog& reqdata)
    {
        try
        {
            msgpack::unpacked msg;
            msgpack::unpack(&msg, packed_data.data(), packed_data.size());
            msg.get().convert(&reqdata);
        }
        catch(const std::exception& e)
        {
            std::cout << "unpack request data failed: " << e.what() << endl;
            return false;
        }
        return true;
    }

    bool appendReqData(const std::string& req_packed_data)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        if (prepared_req_.empty())
            return false;
        const CommonReqData& reqdata = prepared_req_.back();
        if (reqdata.inc_id < last_writed_id_)
        {
            std::cout << "append error!!! Request log must append in order by inc_id. " << std::endl;
            return false;
        }
        assert(reqdata == req_packed_data.common_data);
        ReqLogHead whead;
        whead.inc_id = reqdata.inc_id;
        whead.reqtype = reqdata.reqtype;
        std::ofstream ofs(getDataPath(whead.inc_id).c_str(), ios::app|ios::binary|ios::ate);
        std::ofstream ofs_head(head_log_path_.c_str(), ios::app|ios::binary|ios::ate);
        if (!ofs.good() || !ofs_head.good())
        {
            std::cerr << "append error!!! Request log open failed. " << std::endl;
            return false;
        }
        whead.req_data_offset = ofs.tellp();
        whead.req_data_len = req_packed_data.size();
        whead.req_data_crc = crc(0, req_packed_data.data(), req_packed_data.size());

        using namespace boost::posix_time;
        static ptime epoch(boost::gregorian::date(1970, 1, 1));
        ptime current_time = microsec_clock::universal_time();
        time_duration td = current_time - epoch;
        std::string time_str = boost::lexical_cast<std::string>(double(td.total_milliseconds())/1000);
        time_str.resize(15, '\0');
        time_str.append('\0');
        memcpy(&whead.reqtime[0], time_str.data(), 16);
        ofs.write(req_packed_data.data(), req_packed_data.size());
        ofs_head.write((const char*)&whead, sizeof(whead));
        last_writed_id_ = whead.inc_id;
        ofs.close();
        ofs_head.close();
        std::cout << "append request log success: " << whead.inc_id << "," << whead.reqtime << std::endl;
        return true;
    }

    inline uint32_t getLastSuccessReqId()
    {
        return last_writed_id_;
    }
    // you can use this to read all request data in sequence.
    bool getReqDataByHeadOffset(size_t& headoffset, ReqLogHead& rethead, std::string& req_packed_data)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        std::ifstream ifs(head_log_path_.c_str(), ios::binary);
        if (!ifs.good())
        {
            std::cerr << "error!!! Request log open failed. " << std::endl;
            throw -1;
            return false;
        }
        ifs.seekg(0, ios::end);
        size_t length = ifs.tellg();
        if (headoffset > length - sizeof(ReqLogHead))
            return false;
        rethead = getHeadData(ifs, headoffset);
        // update offset to next head position.
        headoffset += sizeof(ReqLogHead);
        return getReqPackedDataByHead(rethead, req_packed_data);
    }

    // get request with inc_id or the minimize that not less than inc_id if not exist.
    // if no more request return false.
    bool getReqData(uint32_t& inc_id, ReqLogHead& rethead, size_t& headoffset, std::string& req_packed_data)
    {
        boost::lock_guard<boost::mutex> lock(lock_);
        if (!getHeadOffset(inc_id, rethead, headoffset))
            return false;
        return getReqPackedDataByHead(rethead, req_packed_data);
    }

    uint32_t crc(uint32_t crc, const char* data, const int32_t len)
    {
        int32_t i;
        for (i = 0; i < len; ++i)
        {
            crc = (crc >> 8) ^ _crc32tab[(crc ^ (*data)) & 0xff];
            data++;
        }
        return (crc);
    }


    bool getHeadOffset(uint32_t& inc_id, ReqLogHead& rethead, size_t& headoffset)
    {
        std::ifstream ifs(head_log_path_.c_str(), ios::binary);
        if (!ifs.good())
        {
            std::cerr << "error!!! Request log open failed. " << std::endl;
            throw -1;
            return false;
        }
        ifs.seekg(0, ios::end);
        size_t length = ifs.tellg();
        assert(length%sizeof(ReqLogHead) == 0);
        size_t start = 0;
        size_t end = length/sizeof(ReqLogHead) - 1;
        ReqLogHead cur = getHeadData(ifs, end*sizeof(ReqLogHead));
        if (inc_id > cur.inc_id)
            return false;
        uint32_t ret_id = inc_id;
        while(end >= start)
        {
            size_t mid = (end - start)/2 + start;
            cur = getHeadData(ifs, mid*sizeof(ReqLogHead));
            if (cur.inc_id > inc_id)
            {
                ret_id = cur.inc_id;
                rethead = cur;
                headoffset = mid*sizeof(ReqLogHead);
                end = mid - 1;
            }
            else if (cur.inc_id < inc_id)
            {
                start = mid + 1;
            }
            else
            {
                ret_id = inc_id;
                rethead = cur;
                headoffset = mid*sizeof(ReqLogHead);
                break;
            }
        }
        inc_id = ret_id;
        return true;
    }

private:
    bool getReqPackedDataByHead(const ReqLogHead& head, std::string& req_packed_data)
    {
        std::ifstream ifs_data(getDataPath(head.inc_id).c_str(), ios::binary);
        if (!ifs_data.good())
        {
            std::cerr << "error!!! Request log open failed. " << std::endl;
            throw -1;
            return false;
        }
        req_packed_data.resize(head.req_data_len);
        ifs_data.seekg(head.req_data_offset, ios::beg);
        ifs_data.read((char*)req_packed_data[0], head.req_data_len);
        if (crc(0, req_packed_data.data(), req_packed_data.size()) != head.req_data_crc)
        {
            std::cout << "warning: crc check failed for request log data." << std::endl;
            throw -1;
            return false;
        }
        return true;
    }
    std::string getDataPath(uint32_t inc_id)
    {
        std::stringstream ss;
        ss << base_path_ << "/" << inc_id/1000 << ".req.log";
        return ss.str();
    }

    ReqLogHead getHeadData(std::ifstream& ifs, size_t offset)
    {
        ReqLogHead head;
        assert(offset%sizeof(ReqLogHead) == 0);
        ifs.seekg(offset, ios::beg);
        ifs.read((char*)&head, sizeof(head));
        return head;
    }

    void loadLastData()
    {
        if (boost::filesystem::exists(base_path_))
        {
            std::ifstream ifs(head_log_path_.c_str(), ios::binary);
            if (ifs.good())
            {
                ifs.seekg(0, ios::end);
                int length = ifs.tellg();
                if (length == 0)
                {
                    std::cout << "no request since last down." << std::endl;
                    return;
                }
                else if (length < (int)sizeof(ReqLogHead))
                {
                    std::cout << "read request log head file error. length :" << length << std::endl;
                    throw std::runtime_error("read request log head file error");
                }
                else if ( length % sizeof(ReqLogHead) != 0)
                {
                    std::cout << "The head file is corrupt. need restore from last backup. len:" << length << std::endl;
                    throw std::runtime_error("read request log head file error");
                }
                ifs.seekg(sizeof(ReqLogHead), ios::end);
                ReqLogHead lasthead;
                ifs.read((char*)&lasthead, sizeof(ReqLogHead));
                inc_id_ = lasthead.inc_id;
                std::cout << "loading request log for last request: inc_id : " <<
                    inc_id_ << ", type:" << lasthead.reqtype << std::endl;
                last_writed_id_ = inc_id_;
                ++inc_id_;
            }
        }
        else
            boost::filesystem::create_directories(base_path_);
    }
    std::string base_path_;
    std::string head_log_path_;
    uint32_t  inc_id_;
    uint32_t  last_writed_id_;
    std::vector<CommonReqData> prepared_req_;
    std::map<uint32_t, size_t> cached_head_offset_;
    boost::mutex  lock_;

    static std::set<std::string> write_req_set_;
};

}
#endif

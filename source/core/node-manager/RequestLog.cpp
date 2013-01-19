#include "RequestLog.h"

namespace sf1r
{
std::set<std::string> ReqLogMgr::write_req_set_;

void ReqLogMgr::initWriteRequestSet()
{
    write_req_set_.insert("documents_create");
}

void ReqLogMgr::init(const std::string& basepath)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    inc_id_ = 1;
    last_writed_id_ = 0;
    base_path_ = basepath;
    head_log_path_ = basepath + "/head.req.log";
    std::vector<CommonReqData>().swap(prepared_req_);
    loadLastData();
}

bool ReqLogMgr::prepareReqLog(CommonReqData& prepared_reqdata, bool isprimary)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    if (!prepared_req_.empty())
    {
        std::cerr << "already has a prepared Request, only one write request allowed." << endl;
        return false;
    }
    if (isprimary)
        prepared_reqdata.inc_id = inc_id_++;
    else
    {
        inc_id_ = prepared_reqdata.inc_id + 1;
    }
    //prepared_reqdata.json_data_size = prepared_reqdata.req_json_data.size();
    prepared_req_.push_back(prepared_reqdata);
    printf("prepare request success, isprimary %d, (id:%u, type:%u) ", isprimary,
        prepared_reqdata.inc_id, prepared_reqdata.reqtype);
    std::cout << endl;
    return true;
}

bool ReqLogMgr::getPreparedReqLog(CommonReqData& reqdata)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    if (prepared_req_.empty())
        return false;
    reqdata = prepared_req_.back();
    return true;
}

void ReqLogMgr::delPreparedReqLog()
{
    boost::lock_guard<boost::mutex> lock(lock_);
    std::vector<CommonReqData>().swap(prepared_req_);
}

bool ReqLogMgr::appendReqData(const std::string& req_packed_data)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    if (prepared_req_.empty())
        return false;
    const CommonReqData& reqdata = prepared_req_.back();
    if (reqdata.inc_id < last_writed_id_)
    {
        std::cerr << "append error!!! Request log must append in order by inc_id. " << std::endl;
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
    time_str.append(1, '\0');
    memcpy(&whead.reqtime[0], time_str.data(), sizeof(whead.reqtime));
    ofs.write(req_packed_data.data(), req_packed_data.size());
    ofs_head.write((const char*)&whead, sizeof(whead));
    last_writed_id_ = whead.inc_id;
    ofs.close();
    ofs_head.close();
    std::vector<CommonReqData>().swap(prepared_req_);
    std::cout << "append request log success: " << whead.inc_id << "," << whead.reqtime << std::endl;
    return true;
}

// you can use this to read all request data in sequence.
bool ReqLogMgr::getReqDataByHeadOffset(size_t& headoffset, ReqLogHead& rethead, std::string& req_packed_data)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    std::ifstream ifs(head_log_path_.c_str(), ios::binary);
    if (!ifs.good())
    {
        std::cerr << "error!!! Request log open failed. " << std::endl;
        throw std::runtime_error("open request log failed.");
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
bool ReqLogMgr::getReqData(uint32_t& inc_id, ReqLogHead& rethead, size_t& headoffset, std::string& req_packed_data)
{
    boost::lock_guard<boost::mutex> lock(lock_);
    if (!getHeadOffset(inc_id, rethead, headoffset))
        return false;
    return getReqPackedDataByHead(rethead, req_packed_data);
}

bool ReqLogMgr::getHeadOffset(uint32_t& inc_id, ReqLogHead& rethead, size_t& headoffset)
{
    std::ifstream ifs(head_log_path_.c_str(), ios::binary);
    if (!ifs.good())
    {
        std::cerr << "error!!! Request log open failed. " << std::endl;
        throw std::runtime_error("open request log file failed.");
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
    std::cout << "get head offset by inc_id: " << inc_id << ", returned nearest id:" << ret_id << std::endl;
    inc_id = ret_id;
    return true;
}

bool ReqLogMgr::getReqPackedDataByHead(const ReqLogHead& head, std::string& req_packed_data)
{
    std::ifstream ifs_data(getDataPath(head.inc_id).c_str(), ios::binary);
    if (!ifs_data.good())
    {
        std::cerr << "error!!! Request log open failed. " << std::endl;
        throw std::runtime_error("open request log file failed.");
    }
    req_packed_data.resize(head.req_data_len);
    ifs_data.seekg(head.req_data_offset, ios::beg);
    ifs_data.read((char*)req_packed_data[0], head.req_data_len);
    if (crc(0, req_packed_data.data(), req_packed_data.size()) != head.req_data_crc)
    {
        std::cerr << "warning: crc check failed for request log data." << std::endl;
        throw std::runtime_error("request log data corrupt.");
    }
    return true;
}

std::string ReqLogMgr::getDataPath(uint32_t inc_id)
{
    std::stringstream ss;
    ss << base_path_ << "/" << inc_id/1000 << ".req.log";
    return ss.str();
}

ReqLogHead ReqLogMgr::getHeadData(std::ifstream& ifs, size_t offset)
{
    ReqLogHead head;
    assert(offset%sizeof(ReqLogHead) == 0);
    ifs.seekg(offset, ios::beg);
    ifs.read((char*)&head, sizeof(head));
    return head;
}

void ReqLogMgr::loadLastData()
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
                std::cerr << "no request since last down." << std::endl;
                return;
            }
            else if (length < (int)sizeof(ReqLogHead))
            {
                std::cerr << "read request log head file error. length :" << length << std::endl;
                throw std::runtime_error("read request log head file error");
            }
            else if ( length % sizeof(ReqLogHead) != 0)
            {
                std::cerr << "The head file is corrupt. need restore from last backup. len:" << length << std::endl;
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

}


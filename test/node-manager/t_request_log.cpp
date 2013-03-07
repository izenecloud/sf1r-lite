#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <node-manager/RequestLog.h>
#undef NDEBUG
#define DEBUG
#include <assert.h>

using namespace std;
using namespace sf1r;
namespace bfs = boost::filesystem;

int main()
{
    bfs::remove_all("./test_reqlog");
    ReqLogMgr reqlogmgr;
    reqlogmgr.init("./test_reqlog");

    CommonReqData test_init_data;
    assert(reqlogmgr.getLastSuccessReqId() == 0);
    CommonReqData test_common_data;
    test_common_data.inc_id = 12;
    test_common_data.reqtype = 3;
    test_common_data.req_json_data = "testjson";
    std::string packed_data;
    ReqLogMgr::packReqLogData(test_common_data, packed_data);
    CommonReqData test_out;
    assert(ReqLogMgr::unpackReqLogData(packed_data, test_out));
    assert(test_common_data.inc_id == test_out.inc_id);
    assert(test_common_data.reqtype == test_out.reqtype);
    assert(test_common_data.req_json_data == test_out.req_json_data);

    IndexReqLog test_index_log;
    test_index_log.reqtype = Req_Index;
    test_index_log.req_json_data = "test_json_for_indexreqlog";
    test_index_log.scd_list.push_back("scdfile1");
    test_index_log.scd_list.push_back("scdfile2");
    test_index_log.scd_list.push_back("scdfile3");
    std::string index_packed_data;
    ReqLogMgr::packReqLogData(test_index_log, index_packed_data);
    test_out = test_init_data;
    assert(ReqLogMgr::unpackReqLogData(index_packed_data, test_out));
    assert(test_index_log.inc_id == test_out.inc_id);
    assert(test_index_log.reqtype == test_out.reqtype);
    assert(test_index_log.req_json_data == test_out.req_json_data);
    cout << test_out.req_json_data << endl;

    IndexReqLog test_indexlog_out;
    assert(ReqLogMgr::unpackReqLogData(index_packed_data, test_indexlog_out));
    assert(test_index_log.inc_id == test_indexlog_out.inc_id);
    assert(test_index_log.reqtype == test_indexlog_out.reqtype);
    assert(test_index_log.req_json_data == test_indexlog_out.req_json_data);
    assert(test_index_log.scd_list == test_indexlog_out.scd_list);
    cout << test_indexlog_out.scd_list[2] << endl;

    bool isprimary = true;
    assert( !reqlogmgr.appendTypedReqLog(test_common_data) );

    size_t primary_write_num = 100000;
    size_t non_primary_write_num = 10000;
    for (size_t i = 1; i < primary_write_num; ++i)
    {
        assert(reqlogmgr.prepareReqLog(test_common_data, isprimary));
        ReqLogMgr::packReqLogData(test_common_data, packed_data);
        assert( test_common_data.inc_id == i );

        test_out = test_init_data;
        assert( reqlogmgr.getPreparedReqLog(test_out) );
        assert( test_common_data.inc_id == test_out.inc_id );
        assert( test_common_data.req_json_data == test_out.req_json_data);
        assert( test_common_data.reqtype == test_out.reqtype);
        assert( reqlogmgr.appendTypedReqLog(test_common_data) );
        reqlogmgr.delPreparedReqLog();
        assert( !reqlogmgr.getPreparedReqLog(test_out) );
        assert( reqlogmgr.getLastSuccessReqId() == i );
        uint32_t inc_id = i;
        ReqLogHead head;
        size_t offset = 0;
        std::string ret_packed_data;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == i );
        assert( head.reqtype == test_common_data.reqtype );
        assert( offset == (i - 1)*sizeof(ReqLogHead) );
        assert( reqlogmgr.getReqDataByHeadOffset(offset, head, ret_packed_data) );
        assert( packed_data == ret_packed_data );

        test_out = test_init_data;
        assert( reqlogmgr.unpackReqLogData(ret_packed_data, test_out) );
        assert( test_out.inc_id == test_common_data.inc_id );
        assert( test_out.reqtype == test_common_data.reqtype);
        assert( test_out.req_json_data == test_common_data.req_json_data);
        ReqLogHead head2;
        size_t offset2 = 0;
        std::string ret_packed_data2;
        assert( reqlogmgr.getReqData(inc_id, head2, offset2, ret_packed_data2));
        assert( offset2 == (i - 1)*sizeof(ReqLogHead) );
        assert( ret_packed_data == ret_packed_data2 );
    }
    {
        uint32_t inc_id = 0;
        ReqLogHead head;
        size_t offset = 0;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == 1 && inc_id == 1);
    }
    // test reload
    reqlogmgr.init("./test_reqlog");
    assert( reqlogmgr.getLastSuccessReqId() == primary_write_num - 1 );
    {
        uint32_t inc_id = 0;
        ReqLogHead head;
        size_t offset = 0;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == 1 && inc_id == 1);
    }
    for (size_t i = 1; i < primary_write_num; ++i)
    {
        uint32_t inc_id = i;
        ReqLogHead head;
        size_t offset = 0;
        std::string ret_packed_data;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == i );
        assert( head.reqtype == test_common_data.reqtype );
        assert( offset == (i - 1)*sizeof(ReqLogHead) );
        assert( reqlogmgr.getReqDataByHeadOffset(offset, head, ret_packed_data) );
        //assert( packed_data == ret_packed_data );

        test_out = test_init_data;
        assert( reqlogmgr.unpackReqLogData(ret_packed_data, test_out) );
        assert( test_out.inc_id == i );
        assert( test_out.reqtype == test_common_data.reqtype);
        assert( test_out.req_json_data == test_common_data.req_json_data);
    }

    isprimary = false;

    for (size_t i = 1; i < primary_write_num; i+=500)
    {
        test_common_data.inc_id = i;
        assert(!reqlogmgr.prepareReqLog(test_common_data, isprimary));
        assert( !reqlogmgr.getPreparedReqLog(test_out) );
    }
    for (size_t i = primary_write_num + 1; i < primary_write_num + non_primary_write_num; i+=5)
    {
        test_common_data.inc_id = i;
        assert(reqlogmgr.prepareReqLog(test_common_data, isprimary));
        ReqLogMgr::packReqLogData(test_common_data, packed_data);
        assert( test_common_data.inc_id == i );

        test_out = test_init_data;
        assert( reqlogmgr.getPreparedReqLog(test_out) );
        assert( test_common_data.inc_id == test_out.inc_id );
        assert( test_common_data.req_json_data == test_out.req_json_data);
        assert( test_common_data.reqtype == test_out.reqtype);
        assert( reqlogmgr.appendTypedReqLog(test_common_data) );
        reqlogmgr.delPreparedReqLog();
        assert( !reqlogmgr.getPreparedReqLog(test_out) );
        assert( reqlogmgr.getLastSuccessReqId() == i );
        uint32_t inc_id = i;
        ReqLogHead head;
        size_t offset = 0;
        std::string ret_packed_data;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == i );
        assert( head.reqtype == test_common_data.reqtype );
        assert( reqlogmgr.getReqDataByHeadOffset(offset, head, ret_packed_data) );
        assert( packed_data == ret_packed_data );

        test_out = test_init_data;
        assert( reqlogmgr.unpackReqLogData(ret_packed_data, test_out) );
        assert( test_out.inc_id == test_common_data.inc_id );
        assert( test_out.reqtype == test_common_data.reqtype);
        assert( test_out.req_json_data == test_common_data.req_json_data);

        ReqLogHead head2;
        size_t offset2 = 0;
        std::string ret_packed_data2;
        assert( reqlogmgr.getReqData(inc_id, head2, offset2, ret_packed_data2));
        assert( ret_packed_data == ret_packed_data2 );
    }
    {
        uint32_t inc_id = 0;
        ReqLogHead head;
        size_t offset = 0;
        assert( reqlogmgr.getHeadOffset(inc_id, head, offset) );
        assert( head.inc_id == 1 && inc_id == 1);
    }
    for (size_t i = primary_write_num + 1; i < primary_write_num + 1 + non_primary_write_num - 10; i+=5)
    {
        uint32_t inc_id_before = i - 1;
        uint32_t inc_id_after = i + 1;
        ReqLogHead head;
        size_t offset = 0;
        std::string ret_packed_data;
        assert( reqlogmgr.getHeadOffset(inc_id_before, head, offset) );
        assert( head.inc_id == i );

        ReqLogHead head2;
        size_t offset2 = 0;
        std::string ret_packed_data2;
        assert( reqlogmgr.getReqData(inc_id_after, head2, offset2, ret_packed_data2));
        assert(head2.inc_id == i + 5);
    }

}


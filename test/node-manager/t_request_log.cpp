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


    bool isprimary = true;
    assert( !reqlogmgr.appendTypedReqLog(test_common_data) );

    for (size_t i = 1; i < 10; ++i)
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
    // test reload
    reqlogmgr.init("./test_reqlog");
    assert( reqlogmgr.getLastSuccessReqId() == 9 );
    for (size_t i = 1; i < 10; ++i)
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
}


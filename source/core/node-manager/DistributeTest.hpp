#ifndef SF1R_DISTRIBUTE_TEST_H
#define SF1R_DISTRIBUTE_TEST_H

#include "ZooKeeperNamespace.h"
#include "ZooKeeperManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace bp = boost::posix_time;

namespace sf1r
{

enum TestFailType
{
    NoAnyTest = 0,
    NoFail,
    PrimaryFail_At_Electing,
    PrimaryFail_At_BeginReqProcess,
    PrimaryFail_At_ReqProcessing,
    PrimaryFail_At_NotifyMasterReadyForNew,
    PrimaryFail_At_AbortReq,
    PrimaryFail_At_FinishReqLocal,
    PrimaryFail_At_Wait_Replica_Abort,
    PrimaryFail_At_Wait_Replica_FinishReq,
    PrimaryFail_At_Wait_Replica_FinishReqLog,
    PrimaryFail_At_Wait_Replica_Recovery,

    ReplicaFail_At_Electing,
    ReplicaFail_At_Recovering,
    ReplicaFail_At_BeginReqProcess,
    ReplicaFail_At_ReqProcessing,
    ReplicaFail_At_Waiting_Primary,
    ReplicaFail_At_Waiting_Primary_Abort,
    ReplicaFail_At_AbortReq,
    ReplicaFail_At_FinishReqLocal,
    ReplicaFail_At_UnpackPrimaryReq,
    ReplicaFail_At_Waiting_Recovery,
    Fail_At_AfterEnterCluster,
};

class DistributeTestSuit
{
public:
    static void loadTestConf()
    {
        current_test_fail_type_ = NoAnyTest;
        std::ifstream conf("./distribute_test.conf");
        if (conf.good())
        {
            std::string line;
            getline(conf, line);
            try
            {
                current_test_fail_type_ = boost::lexical_cast<int>(line);
            }
            catch(const std::exception& e)
            {
            }
        }
        std::cout << "current sf1r node fail test type is : " << current_test_fail_type_ << std::endl;
    }
    static void testFail(TestFailType failtype)
    {
        if (current_test_fail_type_ == NoAnyTest ||
            current_test_fail_type_ == NoFail)
            return;
        if (current_test_fail_type_ == failtype)
        {
            std::cout << "current node failed at test point: " << failtype << std::endl;
            testForceExit();
        }
    }

    static void updateMemoryState(const std::string& key, int value)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        updateMemoryState(key, boost::lexical_cast<std::string>(value));
    }

    static void updateMemoryState(const std::string& key, const std::string& value)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        memory_state_info_[key] = value;
    }

    static void clearMemoryState(const std::string& key)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        memory_state_info_.erase(key);
    }

    static void getMemoryState(const std::vector<std::string>& keylist, std::vector<std::string>& valuelist)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        std::vector<std::string>().swap(valuelist);
        valuelist.resize(keylist.size());
        for(size_t i = 0; i < keylist.size(); ++i)
        {
            std::map<std::string, std::string>::const_iterator cit = memory_state_info_.find(keylist[i]);
            if (cit != memory_state_info_.end())
            {
                valuelist[i] = cit->second;
            }
        }
    }

    static void getMemoryStateKeyList(std::vector<std::string>& keylist)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        std::vector<std::string>().swap(keylist);
        keylist.reserve(memory_state_info_.size());
        std::map<std::string, std::string>::const_iterator cit = memory_state_info_.begin();
        while(cit != memory_state_info_.end())
        {
            keylist.push_back(cit->first);
            ++cit;
        }
    }

    static std::string getStatusReport()
    {
        if (current_test_fail_type_ == NoAnyTest)
            return "";
        std::stringstream ss;
        std::map<std::string, std::string>::const_iterator cit = memory_state_info_.begin();
        ss << "Memory state info : " << std::endl;
        while(cit != memory_state_info_.end())
        {
            ss << cit->first << " : " << cit->second << std::endl;
            ++cit;
        }
        return ss.str();
    }

    static void reportCurrentStateToFile()
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        // print all the node data in current sf1r cluster
        std::string report_file = "./distribute_test_report_";
        report_file += bp::to_iso_string(bp::second_clock::local_time());
        report_file += ".data";
        std::ofstream ofs(report_file.c_str());
        ofs << getStatusReport();
    }

private:
    static void testForceExit()
    {
        int *p = NULL;
        *p = 10;
        throw -1;
    }
    static int current_test_fail_type_;
    static std::map<std::string, std::string> memory_state_info_;

};

}

#endif

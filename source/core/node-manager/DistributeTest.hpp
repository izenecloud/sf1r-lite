#ifndef SF1R_DISTRIBUTE_TEST_H
#define SF1R_DISTRIBUTE_TEST_H

#include "ZooKeeperNamespace.h"
#include "ZooKeeperManager.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>

namespace sf1r
{

class DistributeTestSuit
{
public:
    enum TestFailType
    {
        NoAnyTest = 0,
        NoFail,
        PrimaryFail_At_Electing,
        PrimaryFail_At_Process_Req,
        ReplicaFail_At_Waiting_Primary,
    };
    static void loadTestConf()
    {
        current_test_fail_type_ = NoAnyTest;
        std::ifstream conf("./distribute_test.conf");
        if (conf.good())
        {
            std::string line;
            getline(conf, line);
            if (line == "NoFail")
            {
                current_test_fail_type_ = NoFail;
            }
            else if (line == "PrimaryFail_At_Electing")
                current_test_fail_type_ = PrimaryFail_At_Electing;
            else if (line == "PrimaryFail_At_Process_Req")
                current_test_fail_type_ = PrimaryFail_At_Process_Req;
        }
        std::cout << "current sf1r node fail test type is : " << current_test_fail_type_ << std::endl;
    }
    static void testFail(TestFailType failtype)
    {
        if (current_test_fail_type_ == NoAnyTest ||
            current_test_fail_type_ == NoFail)
            return;
        if (current_test_fail_type_ == failtype)
            testForceExit();
    }
    static std::string genTestDataForCurrentNode()
    {
        if (current_test_fail_type_ == NoAnyTest)
            return "";
        // scan the data on disk, generate the detail report used for compare with others.
    }
    static std::string getTestData(int nodeid)
    {
        if (current_test_fail_type_ == NoAnyTest)
            return "";
    }
    static void reportCurrentState()
    {
        if (current_test_fail_type_ == NoAnyTest)
            return;
        // print all the node data in current sf1r cluster
    }

private:
    static void testForceExit()
    {
        int *p = NULL;
        *p = 10;
        throw -1;
    }
    static int current_test_fail_type_;

};

int DistributeTestSuit::current_test_fail_type_ = NoAnyTest;

}

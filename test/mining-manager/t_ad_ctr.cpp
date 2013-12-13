#include <mining-manager/ad-index-manager/AdClickPredictor.h>
#include <mining-manager/ad-index-manager/AdStreamSubscriber.h>
#include <node-manager/SuperNodeManager.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <glog/logging.h>

namespace bfs=boost::filesystem;
using namespace sf1r;
static std::vector<std::vector<std::pair<std::string, std::string> > > predict_test_data;
const int thread_num = 8;
const size_t attr_name_size = 100;
const size_t attr_value_size = 1000;
const size_t train_num = 5000;
const size_t test_num = 10000;

void predict_func(AdClickPredictor* pad)
{
    LOG(INFO) << "begin predict test.";
    double totalret = 0;
    for(size_t i = 0; i < predict_test_data.size(); ++i)
    {
        totalret += pad->predict(predict_test_data[i]);
    }
    LOG(INFO) << "end test, total ret : " << totalret;
}

void training_func(AdClickPredictor* pad, std::vector<std::string>* attr_name_list,
    std::vector<std::string>* attr_value_list)
{
    for (size_t i = 0; i < test_num/100; ++i)
    {
        std::vector<std::pair<std::string, std::string> > item;
        size_t attrnum = rand() % 10 + 1;
        item.resize(attrnum);
        for (size_t j = 0; j < attrnum; ++j)
        {
            item[j].first = (*attr_name_list)[rand()%(*attr_name_list).size()];
            item[j].second = (*attr_value_list)[rand()%(*attr_value_list).size()];
        }
        pad->update(item, (attrnum%2 == 0));
        usleep(1000);
    }
    LOG(INFO) << "update finished.";
}

std::string gen_rand_str()
{
    size_t rand_len = rand() % 100 + 3;
    std::string ret(rand_len, '\0');
    for(size_t i = 0; i < rand_len; ++i)
    {
        int r = rand();
        ret[i] = ((r%2 == 0) ?'a':'A') + r%26;
    }
    return ret;
}

void stream_update_func()
{
    std::vector<AdMessage> msglist;
    for(size_t i = 0; i < 100; i++)
    {
        AdMessage m;
        m.topic = "AdClickLog";
        m.body = "test";
        msglist.push_back(m);
    }
    for(size_t i = 0; i < train_num; ++i)
    {
        AdStreamSubscriber::get()->onAdMessage(msglist);
    }
    LOG(INFO) << "stream update finished.";
}

int main()
{

    srand(time(NULL));
    if (!bfs::exists("/opt/mine/ad_ctr_test"))
        return 0;
    bfs::remove_all("/opt/mine/ad_ctr_test/data");
    bfs::create_directories("/opt/mine/ad_ctr_test/data");

    LOG(INFO) << "begin generate training test data.";
    std::ifstream ifs("/opt/mine/track2/training.txt");
    // generate the data.
    std::ofstream ofs("/opt/mine/ad_ctr_test/data/1.txt");
    //std::vector<std::string> attr_name_list;
    //attr_name_list.reserve(attr_name_size);
    //for (size_t i = 0; i < attr_name_size; ++i)
    //{
    //    attr_name_list.push_back(gen_rand_str());
    //}
    //std::vector<std::string> attr_value_list;
    //attr_value_list.reserve(attr_value_size);
    //for (size_t i = 0; i < attr_value_size; ++i)
    //{
    //    attr_value_list.push_back(gen_rand_str());
    //}
    //for (size_t i = 0; i < train_num; ++i)
    //{
    //    size_t attrnum = rand() % 10 + 1;
    //    for (size_t j = 0; j < attrnum; ++j)
    //    {
    //        ofs << attr_name_list[rand()%attr_name_list.size()] << ":"
    //            << attr_value_list[rand()%attr_value_list.size()] << " ";
    //    }
    //    ofs << ((attrnum%2 == 0)?"0":"1") << std::endl;
    //}
    std::vector<std::string> attr_name_list;
    size_t training_readed = 0;
    attr_name_list.push_back("DisplayURL");
    attr_name_list.push_back("AdID");
    attr_name_list.push_back("AdvertiserID");
    attr_name_list.push_back("Depth");
    attr_name_list.push_back("Position");
    attr_name_list.push_back("QueryID");
    attr_name_list.push_back("KeywordID");
    attr_name_list.push_back("TitleID");
    attr_name_list.push_back("DescriptionID");
    attr_name_list.push_back("UserID");
    while(ifs.good() && training_readed < train_num)
    {
        std::vector<std::string> attr_value_list(attr_name_list.size());
        uint32_t clicked = 0;
        uint32_t impressioned = 0;
        ifs >> clicked >> impressioned;
        for(size_t i = 0; i < attr_value_list.size(); ++i)
        {
            ifs >> attr_value_list[i];
        }
        for (size_t click_num = 0; click_num < clicked; ++click_num)
        {
            for(size_t j = 0; j < attr_name_list.size(); ++j)
            {
                ofs << attr_name_list[j] << ":" << attr_value_list[j] << " ";
            }
            ofs << "1" << std::endl;
        }
        for (size_t click_num = 0; click_num < impressioned; ++click_num)
        {
            for(size_t j = 0; j < attr_name_list.size(); ++j)
            {
                ofs << attr_name_list[j] << ":" << attr_value_list[j] << " ";
            }
            ofs << "0" << std::endl;
        }
        ++training_readed;
        if (training_readed % 1000000 == 0)
        {
            LOG(INFO) << "trained: " << training_readed;
            ofs.flush();
        }
    }
    ofs.close();
    ifs.close();

    DistributedCommonConfig config;
    config.localHost_ = "172.16.5.11";
    SuperNodeManager::get()->init(config);

    AdStreamSubscriber::get()->init("172.16.5.30", 19850);

    AdClickPredictor ad;
    ad.init("/opt/mine/ad_ctr_test");
    LOG(INFO) << "begin training data.";
    ad.preProcess();
    ad.train();
    ad.postProcess();

    LOG(INFO) << "begin generate predict test data.";
    // load test data from file.
    predict_test_data.reserve(test_num);
    //for (size_t i = 0; i < test_num; ++i)
    //{
    //    predict_test_data.push_back(std::vector<std::pair<std::string, std::string> >());
    //    //size_t attrnum = rand()%10 + 1;
    //    for(size_t j = 0 ; j < attrnum; ++j)
    //    {
    //        predict_test_data.back().push_back(std::make_pair(attr_name_list[rand()%attr_name_list.size()], attr_value_list[rand()%attr_value_list.size()]));
    //    }
    //}
    std::ifstream ifs_test("/opt/mine/test.txt");
    size_t cnt = 0;
    while(ifs_test.good() && cnt < test_num)
    {
        std::vector<std::string> attr_value_list(attr_name_list.size());
        for(size_t i = 0; i < attr_value_list.size(); ++i)
        {
            ifs_test >> attr_value_list[i];
        }
        predict_test_data.push_back(std::vector<std::pair<std::string, std::string> >());
        for(size_t j = 0; j < attr_name_list.size(); ++j)
        {
            predict_test_data.back().push_back(std::make_pair(attr_name_list[j], attr_value_list[j]));
        }
        ++cnt;
    }

    //boost::thread* write_thread = new boost::thread(boost::bind(&training_func, &ad, &attr_name_list, &attr_value_list));

    LOG(INFO) << "begin do predict test.";
    boost::thread_group test_threads;
    for(int i = 0; i < thread_num; ++i)
    {
        test_threads.add_thread(new boost::thread(boost::bind(&predict_func, &ad)));
    }
    test_threads.join_all();
    sleep(10000);
    AdStreamSubscriber::get()->stop();
    //write_thread->join();
    //delete write_thread;
    //test_threads.clear();

   // boost::thread_group test_threads_2;
   // LOG(INFO) << "Begin test stream update.";
   // for(int i = 0; i < 4; ++i)
   // {
   //     test_threads_2.add_thread(new boost::thread(boost::bind(&stream_update_func)));
   // }
   // test_threads_2.join_all();
    LOG(INFO) << "all finished.";
}


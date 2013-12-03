/*
 * AdCliclPredictor.cpp
 */

#include "AdClickPredictor.h"
#include "AdStreamSubscriber.h"
#include <dirent.h>
#include <stdio.h>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>


namespace sf1r
{
static const std::string adlog_topic = "b5manlog";

void AdClickPredictor::init(const std::string& path)
{
    predictor_.reset(new AdPredictorType(0, 400, 450, 0.08));

    dataPath_ = path + "/data/";
    modelPath_ = path + "/model/predictor.bin";

    boost::filesystem::create_directories(path + "/model");
    boost::filesystem::create_directories(dataPath_ + "backup");

    load();

    bool ret = AdStreamSubscriber::get()->subscribe(adlog_topic, boost::bind(&AdClickPredictor::onAdStreamMessage, this, _1));
    if (!ret)
    {
        LOG(ERROR) << "subscribe the click log failed !!!!!!";
    }
}

void AdClickPredictor::stop()
{
    // unsubscribe should make sure all callback finished and 
    // no any callback will be send later.
    AdStreamSubscriber::get()->unsubscribe(adlog_topic);
}

void AdClickPredictor::onAdStreamMessage(const std::vector<AdMessage>& msg_list)
{
    std::vector<std::pair<AssignmentT, bool> > assignment_list;
    static int cnt = 0;
    if (cnt % 10000 == 0)
    {
        for (size_t i = 0; i < msg_list.size(); ++i)
        {
            LOG(INFO) << "stream data: " << msg_list[i].body;
        }
        LOG(INFO) << "got ad stream data. size: " << msg_list.size() << ", total " << cnt;
    }
    cnt += msg_list.size();
    // read from stream msg
    for(size_t i = 0; i < assignment_list.size(); ++i)
    {
        update(assignment_list[i].first, assignment_list[i].second);
    }
}

bool AdClickPredictor::preProcess()
{
    //use copy constructor
    learner_.reset(new AdPredictorType(*predictor_));
    return true;
}

//Train model with named file
bool AdClickPredictor::trainFromFile(const std::string& filename)
{
    LOG(INFO) << "training from file: " << filename << std::endl;

    std::string oldname = dataPath_ + filename;

    std::ifstream fin(oldname.c_str());

    std::string str;
    bool click = false;
    while ( getline(fin, str) )
    {
        //parse log
        //LOG(INFO) << "str: " << str << std::endl;

        std::vector<std::string> elems;
        std::vector<std::pair<std::string, std::string> > knowledge;

        boost::split(elems, str, boost::is_any_of(" "));

        std::vector<std::string>::iterator it, end;
        end = elems.end(); end--;

        if ( *end == "0" )
            click = false;
        else if ( *end == "1" )
            click = true;
        else
        {
            LOG(INFO) << "Unknown Log Format : " << str << std::endl;
            continue;
        }

        for (it = elems.begin(); it != end; it++ )
        {
            std::vector<std::string> assignment;
            boost::split(assignment, *it, boost::is_any_of(":"));

            if ( assignment.size() != 2 )
            {
                LOG(INFO) << "Unknown Log Format : " << str << std::endl;
                continue;
            }
            knowledge.push_back(std::make_pair(assignment[0], assignment[1]));
        }
        //learn
        learner_->update(knowledge, click);
    }

    std::string newname = dataPath_ + "backup/"  + filename;

    //move the current file to the backup data path
    if ( rename(oldname.c_str(), newname.c_str()) != 0 )
    {
        LOG(INFO) << "Rename File Error: " << oldname << std::endl;
    }
    return true;
}

bool AdClickPredictor::train()
{
    // Obtain all the data files under the dataPath_
    DIR *dp;
    struct dirent *dirp;

    if ( (dp = opendir(dataPath_.c_str())) == NULL )
    {
        LOG(ERROR) << "opendir error: " << dataPath_ << std::endl;
        return false;
    }

    while ( (dirp = readdir(dp)) != NULL )
    {
        if ( (strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..") == 0))
            continue;
        if ( dirp->d_type == DT_DIR )
            continue;

        std::string filename(dirp->d_name);

        LOG(INFO) << "Train AdClickPredictor from file: " << dirp->d_name << std::endl;

        //Training model with the current data file
        trainFromFile(filename);
    }
    return true;
}

bool AdClickPredictor::postProcess()
{
    //LOG(INFO) << "postProcess after training " << std::endl;
    std::ofstream ofs(modelPath_.c_str(), std::ios_base::binary);
    if (!ofs)
    {
        LOG(ERROR) << "failed openning file " << modelPath_ << std::endl;
        return false;
    }

    try
    {
        learner_->save_binary(ofs);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file " << e.what()
                   << ", path: " << modelPath_ << std::endl;
        return false;
    }

    writeLock lock(rwMutex_);
    predictor_ = learner_;
    learner_.reset();
    return true;
}

bool AdClickPredictor::load()
{
    std::ifstream ifs(modelPath_.c_str(), std::ios_base::binary);
    if (!ifs)
        return false;
    try
    {
        writeLock lock(rwMutex_);
        predictor_->load_binary(ifs);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << modelPath_ << std::endl;
        return false;
    }
    return true;
}

} //namespace sf1r

#include "AdSelector.h"
#include "AdClickPredictor.h"
#include <boost/random.hpp>
#include <search-manager/HitQueue.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

#define MAX_HISTORY_CTR_NUM 1024*1024*16

namespace bfs = boost::filesystem;

namespace sf1r
{

AdSelector::AdSelector()
    :history_ctr_data_(MAX_HISTORY_CTR_NUM)
{
}

AdSelector::~AdSelector()
{
}

void AdSelector::init(const std::string& segments_data_path)
{
    segments_data_path_ = segments_data_path;
    ad_click_predictor_ = AdClickPredictor::get();
    //
    // load default all features from config file.
    std::ifstream ifs_def(std::string(segments_data_path_ + "/all_feature_name.txt").c_str());

    while(ifs_def.good())
    {
        std::string feature_name;
        std::getline(ifs_def, feature_name);
        if (!feature_name.empty())
        {
            default_full_features_[feature_name] = std::vector<std::string>();
            init_counter_[feature_name] = 0;
        }
    }

    if (default_full_features_.empty())
    {
        throw std::runtime_error("no default features");
    }

    std::ifstream ifs(std::string(segments_data_path_ + "/clicked_ad.data").c_str());
    if (ifs.good())
    {
        std::size_t num = 0;
        ifs.read((char*)&num, sizeof(num));
        std::vector<docid_t> clicked_list(num);
        ifs.read((char*)clicked_list[0], sizeof(docid_t)*num);
        for(size_t i = 0; i < clicked_list.size(); ++i)
        {
            clicked_ads_.set(clicked_list[i]);
        }
    }
    std::ifstream ifs_seg(std::string(segments_data_path_ + "/all_segments.data").c_str());
    if (ifs_seg.good())
    {
        std::size_t len = 0;
        ifs_seg.read((char*)&len, sizeof(len));
        std::string data;
        data.resize(len);
        ifs_seg.read((char*)data[0], len);
        izenelib::util::izene_deserialization<FeatureMapT> izd(data.data(), data.size());
        izd.read_image(all_segments_);
    }

    if (all_segments_.empty())
    {
        all_segments_ = default_full_features_;
        for(FeatureMapT::iterator it = all_segments_.begin();
            it != all_segments_.end(); ++it)
        {
            it->second.push_back("Unknown");
        }
    }
    else
    {
        // make sure the default full features is the same size with current loaded segments.
        // remove old abandoned feature and add new added feature.
        FeatureMapT::iterator allseg_it = all_segments_.begin();
        while(allseg_it != all_segments_.end())
        {
            if (default_full_features_.find(allseg_it->first) == default_full_features_.end())
            {
                all_segments_.erase(allseg_it++);
            }
            else
                ++allseg_it;
        }

        std::vector<std::string> tmp;
        tmp.push_back("Unknown");
        for(FeatureMapT::const_iterator it = default_full_features_.begin();
            it != default_full_features_.end(); ++it)
        {
            if (all_segments_.find(it->first) == all_segments_.end())
            {
                all_segments_[it->first] = tmp;
            }
        }
    }

    ctr_update_thread_ = boost::thread(boost::bind(&AdSelector::updateFunc, this));
}

void AdSelector::stop()
{
    ctr_update_thread_.interrupt();
    ctr_update_thread_.join();

    std::ofstream ofs(std::string(segments_data_path_ + "/clicked_ad.data").c_str());
    std::vector<docid_t> clicked_list;
    for(std::size_t i = 0; i < clicked_ads_.size(); ++i)
    {
        if (clicked_ads_.test(i))
            clicked_list.push_back(i);
    }
    std::size_t len = clicked_list.size();
    ofs.write((const char*)&len, sizeof(len));
    if (!clicked_list.empty())
        ofs.write((const char*)&clicked_list[0], sizeof(clicked_list[0])*clicked_list.size());

    std::ofstream ofs_seg(std::string(segments_data_path_ + "/all_segments.data").c_str());
    len = 0;
    char* buf = NULL;
    izenelib::util::izene_serialization<FeatureMapT> izs(all_segments_);
    izs.write_image(buf, len);
    ofs_seg.write((const char*)&len, sizeof(len));
    ofs_seg.write(buf, len);
}

void AdSelector::updateFunc()
{
    while(true)
    {
        try
        {
            computeHistoryCTR();
            struct timespec ts;
            ts.tv_sec = 10;
            ts.tv_nsec = 0;
            uint16_t times = 0;
            while(times++ < 360)
            {
                boost::this_thread::interruption_point();
                nanosleep(&ts, NULL);
            }
        }
        catch(boost::thread_interrupted&)
        {
            LOG(INFO) << "ad selector update thread exited.";
            break;
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << "error in ad selector update thread : " << e.what();
        }
    }
}

void AdSelector::computeHistoryCTR()
{
    // Load new segments from files or from DMP server.
    // Each segment has at least one feature value with "Unknown"
    std::map<std::string, std::set<std::string> > distinct_values;
    for(FeatureMapT::iterator it = all_segments_.begin();
        it != all_segments_.end(); ++it)
    {
        distinct_values[it->first].insert(it->second.begin(), it->second.end());
    }

    LOG(INFO) << "begin compute history ctr.";
    std::string updated_seg_file = segments_data_path_ + "/updated_segments.txt";
    if (bfs::exists(updated_seg_file.c_str()))
    {
        std::ifstream ifs(updated_seg_file.c_str());
        while(ifs.good())
        {
            std::string line;
            std::getline(ifs, line);
            std::size_t delim = line.find(":");
            if (delim != std::string::npos)
            {
                std::string name = line.substr(0, delim);
                std::string value = line.substr(delim + 1);
                FeatureMapT::iterator it = all_segments_.find(name);
                if (it != all_segments_.end() &&
                    distinct_values[name].find(value) == distinct_values[name].end())
                {
                    it->second.push_back(value);
                    distinct_values[name].insert(value);
                }
            }
        }
        bfs::remove(updated_seg_file);
    }

    if (!updated_segments_.empty())
    {
        boost::unique_lock<boost::mutex> lock(segment_mutex_);
        LOG(INFO) << "update new segments : " << updated_segments_.size();
        for(FeatureMapT::const_iterator cit = updated_segments_.begin();
            cit != updated_segments_.end(); ++cit)
        {
            FeatureMapT::iterator seg_it = all_segments_.find(cit->first);
            if (seg_it == all_segments_.end())
                continue;
            for(size_t i = 0; i < cit->second.size(); ++i)
            {
                if (distinct_values[cit->first].find(cit->second[i]) == distinct_values[cit->first].end())
                {
                    seg_it->second.push_back(cit->second[i]);
                    distinct_values[cit->first].insert(cit->second[i]);
                }
            }
        }
        updated_segments_.clear();
    }

    // update the history ctr every few hours.
    // Compute the CTR for each user segment and each ad segment.
    //
    std::map<std::string, std::size_t> segments_counter = init_counter_;
    std::map<std::string, std::size_t>::iterator counter_it = segments_counter.begin();
    assert(segments_counter.size() == all_segments_.size() &&
       all_segments_.size() == default_full_features_.size());


    while(counter_it != segments_counter.end())
    {
        std::string key;
        std::vector<std::pair<std::string, std::string> > segment_data;
        for(FeatureMapT::const_iterator segit = all_segments_.begin();
            segit != all_segments_.end(); ++segit)
        {
            std::string value = segit->second[segments_counter[segit->first]];
            key += value;
            segment_data.push_back(std::make_pair(segit->first, value));
        }
        history_ctr_data_[key] = ad_click_predictor_->predict(segment_data);
        while (counter_it->second >= all_segments_[counter_it->first].size() - 1)
        {
            ++counter_it;
            if (counter_it == segments_counter.end())
                return;
        }
        ++(counter_it->second);
    }
    LOG(INFO) << "update history ctr finished.";
}

void AdSelector::updateSegments(const FeatureT& segments)
{
    boost::unique_lock<boost::mutex> lock(segment_mutex_);
    for(FeatureT::const_iterator cit = segments.begin();
        cit != segments.end(); ++cit)
    {
        FeatureMapT::iterator uit = updated_segments_.find(cit->first);
        if (uit == updated_segments_.end())
            continue;
        uit->second.push_back(cit->second);
    }
}

void AdSelector::updateClicked(docid_t ad_id)
{
    clicked_ads_.set(ad_id);
}

double AdSelector::getHistoryCTR(const FeatureT& user, const FeatureT& ad)
{
    FeatureMapT full_feature_list = default_full_features_;
    for(size_t i = 0; i < user.size(); ++i)
    {
        FeatureMapT::iterator it = full_feature_list.find(user[i].first);
        if (it == full_feature_list.end())
        {
            LOG(WARNING) << "an unknown feature type in user: " << user[i].first << ", value: " << user[i].second ;
            continue;
        }
        it->second.push_back(user[i].second);
    }
    for(size_t i = 0; i < ad.size(); ++i)
    {
        FeatureMapT::iterator it = full_feature_list.find(ad[i].first);
        if (it == full_feature_list.end())
        {
            LOG(WARNING) << "an unknown feature type in ad: " << ad[i].first << ", value: " << ad[i].second ;
            continue;
        }
        it->second.push_back(ad[i].second);
    }

    std::string fullkey;
    std::vector<std::string> all_fullkey;
    all_fullkey.push_back("");
    for(FeatureMapT::const_iterator it = full_feature_list.begin();
        it != full_feature_list.end(); ++it)
    {
        if (it->second.empty())
        {
            for(size_t i = 0; i < all_fullkey.size(); ++i)
            {
                all_fullkey[i] += "|Unknown|";
            }
        }
        else if (it->second.size() == 1)
        {
            for(size_t i = 0; i < all_fullkey.size(); ++i)
            {
                all_fullkey[i] += "|" + it->second[0] + "|";
            }
        }
        else
        {
            // for multi feature values we just get the highest ctr.
            std::size_t oldsize = all_fullkey.size();
            for(size_t i = 0; i < oldsize; ++i)
            {
                for(size_t j = 1; j < it->second.size(); ++j)
                {
                    all_fullkey.push_back(all_fullkey[i]);
                    all_fullkey.back() += "|" + it->second[j] + "|";
                }
                all_fullkey[i] += "|" + it->second[0] + "|";
            }
        }
    }
    double max_ctr = 0;
    boost::unordered_map<std::string, double>::const_iterator it = history_ctr_data_.find(all_fullkey[0]);
    if (it != history_ctr_data_.end())
        max_ctr = it->second;

    if (all_fullkey.size() > 1)
    {
        LOG(INFO) << "multi values : " << all_fullkey.size(); 
        for (size_t i = 1; i < all_fullkey.size(); ++i)
        {
            it = history_ctr_data_.find(all_fullkey[i]);
            if (it != history_ctr_data_.end())
            {
                max_ctr = std::max(it->second, max_ctr);
            }
        }
    }
    return max_ctr;
}

void AdSelector::selectByRandSelectPolicy(std::size_t max_unclicked_retnum, std::vector<docid_t>& unclicked_doclist)
{
    boost::random::mt19937 gen;
    boost::random::uniform_int_distribution<> dist(0, unclicked_doclist.size() - 1);

    std::set<int> rand_index_list;
    for(size_t i = 0; i < max_unclicked_retnum; ++i)
    {
        rand_index_list.insert(dist(gen));
    }
    std::vector<docid_t> tmp_doclist;
    tmp_doclist.reserve(rand_index_list.size());
    for(std::set<int>::const_iterator it = rand_index_list.begin();
        it != rand_index_list.end(); ++it)
    {
        tmp_doclist.push_back(unclicked_doclist[(*it) % unclicked_doclist.size()]);
    }
    tmp_doclist.swap(unclicked_doclist);
}

bool AdSelector::selectFromRecommend(const FeatureT& user_info,
    std::size_t max_return,
    std::vector<docid_t>& recommended_doclist
    )
{
    // select the ads from recommend system if no any search results.
    return false;
}

bool AdSelector::select(const FeatureT& user_info,
    const std::vector<FeatureT>& ad_feature_list, 
    std::size_t max_return,
    std::vector<docid_t>& ad_doclist)
{
    if (ad_feature_list.size() != ad_doclist.size())
    {
        LOG(ERROR) << "ad features not match doclist.";
        return false;
    }
    std::size_t max_clicked_retnum = max_return/3 + 1;
    std::size_t max_unclicked_retnum = max_return - max_clicked_retnum;

    std::map<docid_t, FeatureT> clicked_docfeature_list;
    std::vector<docid_t> unclicked_doclist;
    unclicked_doclist.reserve(ad_doclist.size());

    std::size_t max_tmp_clicked_num = max_clicked_retnum * 10;
    ScoreSortedHitQueue tmp_clicked_scorelist(max_tmp_clicked_num);

    for(size_t i = 0; i < ad_doclist.size(); ++i)
    {
        if (clicked_ads_.test(ad_doclist[i]))
        {
            // for clicked item, we filter the result first by the history ctr.
            clicked_docfeature_list[ad_doclist[i]] = ad_feature_list[i];
            double score = 0;
            if (ad_doclist.size() > max_tmp_clicked_num)
            {
                score = getHistoryCTR(user_info, ad_feature_list[i]);
            }
            ScoreDoc item(ad_doclist[i], score);
            tmp_clicked_scorelist.insert(item);
        }
        else
        {
            unclicked_doclist.push_back(ad_doclist[i]);
        }
    }

    std::size_t scoresize = tmp_clicked_scorelist.size();
    // rank the finally filtered clicked item by realtime CTR. 
    ScoreSortedHitQueue clicked_scorelist(max_clicked_retnum);
    for(int i = scoresize - 1; i >= 0; --i)
    {
        ScoreDoc item;
        item.docId = tmp_clicked_scorelist.pop().docId;
        item.score = ad_click_predictor_->predict(user_info, clicked_docfeature_list[item.docId]);
        clicked_scorelist.insert(item);
    }
    // random select some un-clicked ads.
    if (max_unclicked_retnum < unclicked_doclist.size())
    {
        selectByRandSelectPolicy(max_unclicked_retnum, unclicked_doclist);
    }
    scoresize = clicked_scorelist.size();
    ad_doclist.resize(scoresize + unclicked_doclist.size());
    std::size_t ret_i = 0;
    for(int i = scoresize - 1; i >= 0; --i)
    {
        ad_doclist[ret_i++] = clicked_scorelist.pop().docId;
    }
    for(size_t i = 0; i < unclicked_doclist.size(); ++i)
    {
        ad_doclist[ret_i++] = unclicked_doclist[i];
    }
    return !ad_doclist.empty();
}

}

#include "AdSelector.h"
#include "AdClickPredictor.h"
#include <search-manager/HitQueue.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupManager.h>

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/dynamic_bitset.hpp>

#define MAX_HISTORY_CTR_NUM 1024*1024*16

namespace bfs = boost::filesystem;

namespace sf1r
{

static const std::string linker("-");
static const std::string UnknownStr("Unknown");

static inline void getValueStrFromPropId(faceted::PropValueTable* pvt,
    const faceted::PropValueTable::PropIdList& propids, AdSelector::FeatureValueT& value_list)
{
    //return boost::lexical_cast<std::string>(pvid) + "-";
    value_list.clear();
    // may have multi value.
    std::vector<izenelib::util::UString> value;
    for (size_t k = 0; k < propids.size(); ++k)
    {
        pvt->propValuePath(propids[k], value, false);
        if (value.empty()) continue;
        std::string valuestr;
        value[0].convertString(valuestr, izenelib::util::UString::UTF_8);
        value_list.push_back(valuestr);
    }
}


AdSelector::AdSelector()
    :history_ctr_data_(MAX_HISTORY_CTR_NUM)
     , random_eng_(std::time(NULL))
     , random_gen_(random_eng_, DistributionT())
     , need_refresh_(true)
     , ad_segid_str_data_(Lux::IO::NONCLUSTER)
{
}

AdSelector::~AdSelector()
{
    stop();
}

void AdSelector::loadDef(const std::string& file, FeatureMapT& def_features,
    std::map<std::string, std::size_t>& init_counter)
{
    std::ifstream ifs_def(file.c_str());

    while(ifs_def.good())
    {
        std::string feature_name;
        std::getline(ifs_def, feature_name);
        if (!feature_name.empty())
        {
            def_features[feature_name] = FeatureValueT();
            init_counter[feature_name] = 0;
        }
    }
    if (def_features.empty())
    {
        throw std::runtime_error("no default features in file : " + file);
    }
}

void AdSelector::init(const std::string& res_path,
    const std::string& segments_data_path,
    AdClickPredictor* pad_predictor,
    faceted::GroupManager* grp_mgr)
{
    res_path_ = res_path;
    segments_data_path_ = segments_data_path;
    ad_click_predictor_ = pad_predictor;
    groupManager_ = grp_mgr;
    //
    // load default all features from config file.
    loadDef(res_path_ + "/all_user_feature_name.txt", default_full_features_[UserSeg], init_counter_[UserSeg]);
    loadDef(res_path_ + "/all_ad_feature_name.txt", default_full_features_[AdSeg], init_counter_[AdSeg]);

    load();
    bfs::create_directories(segments_data_path_ + "/segid/");
    ad_segid_mgr_.reset(new AdSegIDManager(segments_data_path_ + "/segid/"));
    ad_segid_str_data_.set_noncluster_params(Lux::IO::Linked);
    ad_segid_str_data_.set_lock_type(Lux::IO::LOCK_THREAD);

    if (bfs::exists(segments_data_path_ + "ad_segid_str.data"))
    {
        ad_segid_str_data_.open(segments_data_path_ + "/ad_segid_str.data", Lux::IO::DB_RDWR);
    }
    else
    {
        ad_segid_str_data_.open(segments_data_path_ + "/ad_segid_str.data", Lux::IO::DB_CREAT);
    }

    all_segments_.resize(TotalSeg);
    for (size_t i = 0; i < TotalSeg; ++i)
    {
        FeatureMapT& segments = all_segments_[i];
        if (segments.empty())
        {
            segments = default_full_features_[i];
            for(FeatureMapT::iterator it = segments.begin();
                it != segments.end(); ++it)
            {
                it->second.push_back(UnknownStr);
            }
        }
        else
        {
            // make sure the default full features is the same size with current loaded segments.
            // remove old abandoned feature and add new added feature.
            FeatureMapT::iterator allseg_it = segments.begin();
            while(allseg_it != segments.end())
            {
                if (default_full_features_[i].find(allseg_it->first) == default_full_features_[i].end())
                {
                    segments.erase(allseg_it++);
                }
                else
                    ++allseg_it;
            }

            FeatureValueT tmp;
            tmp.push_back(UnknownStr);
            for(FeatureMapT::const_iterator it = default_full_features_[i].begin();
                it != default_full_features_[i].end(); ++it)
            {
                if (segments.find(it->first) == segments.end())
                {
                    segments[it->first] = tmp;
                }
            }
        }
    }
    ctr_update_thread_ = boost::thread(boost::bind(&AdSelector::updateFunc, this));
}

void AdSelector::getDefaultFeatures(FeatureMapT& feature_name_list, SegType type)
{
    feature_name_list = default_full_features_[type];
}

void AdSelector::load()
{
    std::ifstream ifs(std::string(segments_data_path_ + "/clicked_ad.data").c_str());
    if (ifs.good())
    {
        std::size_t num = 0;
        ifs.read((char*)&num, sizeof(num));
        std::vector<docid_t> clicked_list;
        clicked_list.resize(num);
        ifs.read((char*)&clicked_list[0], sizeof(clicked_list[0])*num);
        for(size_t i = 0; i < clicked_list.size(); ++i)
        {
            clicked_ads_.set(clicked_list[i]);
        }
    }
    LOG(INFO) << "clicked data loaded, total clicked num: " << clicked_ads_.count();
    std::ifstream ifs_seg(std::string(segments_data_path_ + "/all_segments.data").c_str());
    if (ifs_seg.good())
    {
        std::size_t len = 0;
        ifs_seg.read((char*)&len, sizeof(len));
        std::string data;
        data.resize(len);
        ifs_seg.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<std::vector<FeatureMapT> > izd(data.data(), data.size());
        izd.read_image(all_segments_);
    }
    LOG(INFO) << "segments loaded: " << all_segments_.size();

    std::ifstream ifs_segid_data(std::string(segments_data_path_ + "/ad_segid.data").c_str());
    if (ifs_segid_data.good())
    {
        std::size_t len = 0;
        ifs_segid_data.read((char*)&len, sizeof(len));
        std::string data;
        data.resize(len);
        ifs_segid_data.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<std::vector<std::vector<SegIdT> > > izd(data.data(), data.size());
        izd.read_image(ad_segid_data_);
    }
    LOG(INFO) << "ad_segid data loaded: " << ad_segid_data_.size();
}

void AdSelector::save()
{
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
    ofs.flush();

    std::ofstream ofs_seg(std::string(segments_data_path_ + "/all_segments.data").c_str());
    len = 0;
    char* buf = NULL;
    izenelib::util::izene_serialization<std::vector<FeatureMapT> > izs(all_segments_);
    izs.write_image(buf, len);
    ofs_seg.write((const char*)&len, sizeof(len));
    ofs_seg.write(buf, len);
    ofs_seg.flush();

    ad_segid_mgr_->flush();

    boost::shared_lock<boost::shared_mutex> lock(ad_segid_mutex_);
    std::ofstream ofs_segid_data(std::string(segments_data_path_ + "/ad_segid.data").c_str());
    len = 0;
    buf = NULL;
    izenelib::util::izene_serialization<std::vector<std::vector<SegIdT> > > izs_segid_data(ad_segid_data_);
    izs_segid_data.write_image(buf, len);
    ofs_segid_data.write((const char*)&len, sizeof(len));
    ofs_segid_data.write(buf, len);
    ofs_segid_data.flush();
}

void AdSelector::stop()
{
    ctr_update_thread_.interrupt();
    ctr_update_thread_.join();

    save();
    ad_segid_str_data_.close();
}

// one ad may have belong to multi segments.
void AdSelector::getAdSegmentStrList(docid_t ad_id, std::vector<std::string>& retstr_list)
{
    if (ad_id > ad_segid_data_.size())
        return;
    boost::shared_lock<boost::shared_mutex> lock(ad_segid_mutex_);
    const std::vector<SegIdT>& segids = ad_segid_data_[ad_id];
    for(size_t i = 0; i < segids.size(); ++i)
    {
        Lux::IO::data_t *val_p = NULL;
        bool ret = ad_segid_str_data_.get(segids[i], &val_p, Lux::IO::SYSTEM);
        if (ret && val_p->size > 0)
        {
            retstr_list.push_back(std::string());
            retstr_list.back().assign((const char*)val_p->data, val_p->size);
        }
        ad_segid_str_data_.clean_data(val_p);
    }
}

void AdSelector::updateAdSegmentStr(docid_t ad_docid, const FeatureMapT& ad_feature, std::vector<SegIdT>& segids)
{
    std::vector<std::string> seg_str_list;
    seg_str_list.push_back("");
    expandSegmentStr(seg_str_list, ad_feature);
    segids.clear();
    for(size_t j = 0; j < seg_str_list.size(); ++j)
    {
        if (seg_str_list[j].empty())
            continue;
        SegIdT segid;
        ad_segid_mgr_->getDocIdByDocName(seg_str_list[j], segid, true);
        segids.push_back(segid);
        ad_segid_str_data_.put(segid, seg_str_list[j].data(), seg_str_list.size(), Lux::IO::OVERWRITE);
    }
    if (ad_docid >= ad_segid_data_.size())
    {
        boost::unique_lock<boost::shared_mutex> guard;
        if (ad_docid >= ad_segid_data_.size())
            ad_segid_data_.resize(ad_docid + 1);
    }
    boost::shared_lock<boost::shared_mutex> lock(ad_segid_mutex_);
    ad_segid_data_[ad_docid].swap(segids);
}

void AdSelector::updateAdSegmentStr(const std::vector<docid_t>& ad_doclist, const std::vector<FeatureMapT>& ad_feature_list)
{
    assert(ad_doclist.size() == ad_feature_list.size());
    std::vector<SegIdT> segids;
    for(size_t i = 0; i < ad_doclist.size(); ++i)
    {
        updateAdSegmentStr(ad_doclist[i], ad_feature_list[i], segids);
    }
}

void AdSelector::miningAdSegmentStr(docid_t startid, docid_t endid)
{
    PropSharedLockSet propSharedLockSet;
    std::vector<faceted::PropValueTable*> pvt_list;
    std::vector<std::string>  prop_name;
    std::vector<std::set<std::string> > all_kinds_ad_segments;
    const FeatureMapT& def_feature_names = default_full_features_[AdSeg];
    for (FeatureMapT::const_iterator feature_it = def_feature_names.begin();
        feature_it != def_feature_names.end(); ++feature_it)
    {
        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(feature_it->first);
        if (pvt)
            propSharedLockSet.insertSharedLock(pvt);
        pvt_list.push_back(pvt);
        prop_name.push_back(feature_it->first);
    }
    all_kinds_ad_segments.resize(pvt_list.size());

    faceted::PropValueTable::PropIdList propids;
    FeatureValueT value_list;
    std::vector<SegIdT> segids;
    for (docid_t i = startid; i <= endid; ++i)
    {
        FeatureMapT ad_feature;
        for (std::size_t feature_index = 0; feature_index < pvt_list.size(); ++feature_index)
        {
            if (pvt_list[feature_index])
            {
                pvt_list[feature_index]->getPropIdList(i, propids);
                getValueStrFromPropId(pvt_list[feature_index], propids, value_list);
                ad_feature[prop_name[feature_index]] = value_list;
                all_kinds_ad_segments[feature_index].insert(value_list.begin(), value_list.end());
            }
        }
        updateAdSegmentStr(i, ad_feature, segids);
        if (i % 10000 == 0)
        {
            LOG(INFO) << "ad segment mining :" << i;
        }
    }

    FeatureMapT::const_iterator it = def_feature_names.begin();
    for (std::size_t i = 0; i < all_kinds_ad_segments.size(); ++i)
    {
        LOG(INFO) << "found total segments : " << all_kinds_ad_segments[i].size() << "  for : " << it->first;
        updateSegments(it->first, all_kinds_ad_segments[i], AdSeg);
    }
}

void AdSelector::updateFunc()
{
    sleep(10);
    while(true)
    {
        try
        {
            computeHistoryCTR();
            need_refresh_ = false;
            save();
            struct timespec ts;
            ts.tv_sec = 10;
            ts.tv_nsec = 0;
            uint16_t times = 0;
            while(times++ < 360)
            {
                boost::this_thread::interruption_point();
                if (need_refresh_)
                    break;
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

void AdSelector::getAllPossibleSegStr(const FeatureMapT& segments,
    std::map<std::string, std::size_t> segments_counter,
    std::vector<std::pair<std::string, FeatureT> >& all_keys)
{
    assert(segments_counter.size() == segments.size());

    std::map<std::string, std::size_t>::reverse_iterator counter_it = segments_counter.rbegin();
    std::map<std::string, std::size_t>::reverse_iterator inc_counter_it = segments_counter.rbegin();

    FeatureT segment_data;
    while(inc_counter_it != segments_counter.rend())
    {
        std::string key;
        segment_data.clear();
        for(FeatureMapT::const_iterator segit = segments.begin();
            segit != segments.end(); ++segit)
        {
            const std::string& value = segit->second[segments_counter[segit->first]];
            if (value != UnknownStr)
            {
                key += value + linker;
                segment_data.push_back(std::make_pair(segit->first, value));
            }
        }
        all_keys.push_back(std::make_pair(key, segment_data));
        FeatureMapT::const_iterator finded_it = segments.find(counter_it->first);
        while (counter_it->second >= finded_it->second.size() - 1)
        {
            counter_it->second = 0;
            ++counter_it;
            if (counter_it == segments_counter.rend())
                break;
            finded_it = segments.find(counter_it->first);
        }
        if (counter_it != segments_counter.rend())
            ++(counter_it->second);
        else
            break;
        counter_it = inc_counter_it;
    }
}

void AdSelector::computeHistoryCTR()
{
    // Load new segments from files or from DMP server.
    std::map<std::string, std::set<std::string> > distinct_values[all_segments_.size()];
    for (size_t seg_index = 0; seg_index < all_segments_.size(); ++seg_index)
    {
        for(FeatureMapT::iterator it = all_segments_[seg_index].begin();
            it != all_segments_[seg_index].end(); ++it)
        {
            distinct_values[seg_index][it->first].insert(it->second.begin(), it->second.end());
            LOG(INFO) << "seg type: " << seg_index << ", seg_name:" << it->first << ", segment value num: " << it->second.size();
        }

        if (!updated_segments_[seg_index].empty())
        {
            boost::unique_lock<boost::mutex> lock(segment_mutex_);
            LOG(INFO) << "update new segments : " << updated_segments_[seg_index].size();
            for(FeatureMapT::const_iterator cit = updated_segments_[seg_index].begin();
                cit != updated_segments_[seg_index].end(); ++cit)
            {
                LOG(INFO) << "key: " << cit->first << ", new value num : " << cit->second.size();
                FeatureMapT::iterator seg_it = all_segments_[seg_index].find(cit->first);
                if (seg_it == all_segments_[seg_index].end())
                    continue;
                for(size_t i = 0; i < cit->second.size(); ++i)
                {
                    if (distinct_values[seg_index][cit->first].find(cit->second[i]) == distinct_values[seg_index][cit->first].end())
                    {
                        seg_it->second.push_back(cit->second[i]);
                        distinct_values[seg_index][cit->first].insert(cit->second[i]);
                    }
                }
            }
            updated_segments_[seg_index].clear();
        }
    }

    LOG(INFO) << "begin compute history ctr.";
    // update the history ctr every few hours.
    // Compute the CTR for each user segment and each ad segment.
    //
    std::string history_ctr_file = segments_data_path_ + "/history_ctr.txt";
    std::ofstream ofs_history(history_ctr_file.c_str());

    std::vector<std::pair<std::string, FeatureT> > all_fullkey[TotalSeg];
    getAllPossibleSegStr(all_segments_[UserSeg], init_counter_[UserSeg], all_fullkey[UserSeg]);
    getAllPossibleSegStr(all_segments_[AdSeg], init_counter_[AdSeg], all_fullkey[AdSeg]);

    for (size_t i = 0; i < all_fullkey[UserSeg].size(); ++i)
    {
        for (size_t j = 0; j < all_fullkey[AdSeg].size(); ++j)
        {
            std::string ad_seg_str;
            if (!all_fullkey[AdSeg][j].first.empty())
            {
                SegIdT segid = 0;
                ad_segid_mgr_->getDocIdByDocName(all_fullkey[AdSeg][j].first, segid, true);
                ad_seg_str = boost::lexical_cast<std::string>(segid);
            }
            double ctr_res = ad_click_predictor_->predict(all_fullkey[UserSeg][i].second, all_fullkey[AdSeg][j].second);
            std::string key = all_fullkey[UserSeg][i].first + ad_seg_str;
            history_ctr_data_[key] = ctr_res;
            ofs_history << key << " : " << ctr_res << std::endl;
        }
    }
    ofs_history.flush();
    LOG(INFO) << "update history ctr finished. total : " << history_ctr_data_.size();
}

void AdSelector::updateSegments(const std::string& segment_name, const std::set<std::string>& segments, SegType type)
{
    boost::unique_lock<boost::mutex> lock(segment_mutex_);
    need_refresh_ = true;
    std::pair<FeatureMapT::iterator, bool> inserted_it = updated_segments_[type].insert(std::make_pair(segment_name, std::vector<std::string>()));
    for(std::set<std::string>::const_iterator cit = segments.begin(); cit != segments.end(); ++cit)
    {
        inserted_it.first->second.push_back(*cit);
    }
}

void AdSelector::updateSegments(const FeatureT& segments, SegType type)
{
    boost::unique_lock<boost::mutex> lock(segment_mutex_);
    for(FeatureT::const_iterator cit = segments.begin();
        cit != segments.end(); ++cit)
    {
        //FeatureMapT::iterator uit = updated_segments_.find(cit->first);
        //if (uit == updated_segments_.end())
        //    continue;
        updated_segments_[type][cit->first].push_back(cit->second);
    }
}

void AdSelector::updateClicked(docid_t ad_id)
{
    clicked_ads_.set(ad_id);
}

void AdSelector::expandSegmentStr(std::vector<std::string>& seg_str_list, const std::vector<SegIdT>& ad_segid_list)
{
    if (ad_segid_list.empty())
    {
        // ignored feature. Any feature value.
    }
    else if (ad_segid_list.size() == 1)
    {
        for(size_t i = 0; i < seg_str_list.size(); ++i)
        {
            seg_str_list[i] += boost::lexical_cast<std::string>(ad_segid_list[0]);
        }
    }
    else
    {
        // for multi feature values we just get the highest ctr.
        std::size_t oldsize = seg_str_list.size();
        for(size_t i = 0; i < oldsize; ++i)
        {
            for(size_t j = 1; j < ad_segid_list.size(); ++j)
            {
                seg_str_list.push_back(seg_str_list[i]);
                seg_str_list.back() += boost::lexical_cast<std::string>(ad_segid_list[j]);
            }
            seg_str_list[i] += boost::lexical_cast<std::string>(ad_segid_list[0]);
        }
    }
}

void AdSelector::expandSegmentStr(std::vector<std::string>& seg_str_list, const FeatureMapT& feature_list)
{
    for(FeatureMapT::const_iterator it = feature_list.begin();
        it != feature_list.end(); ++it)
    {
        if (it->second.empty())
        {
            // ignored feature. Any feature value.
        }
        else if (it->second.size() == 1)
        {
            for(size_t i = 0; i < seg_str_list.size(); ++i)
            {
                seg_str_list[i] += it->second[0] + linker;
            }
        }
        else
        {
            // for multi feature values we just get the highest ctr.
            std::size_t oldsize = seg_str_list.size();
            for(size_t i = 0; i < oldsize; ++i)
            {
                for(size_t j = 1; j < it->second.size(); ++j)
                {
                    seg_str_list.push_back(seg_str_list[i]);
                    seg_str_list.back() += it->second[j] + linker;
                }
                seg_str_list[i] += it->second[0] + linker;
            }
        }
    }
}

void AdSelector::getUserSegmentStr(std::vector<std::string>& user_seg_str_list, const FeatureT& user_info)
{
    FeatureMapT user_feature_list;
    for (size_t i = 0; i < user_info.size(); ++i)
    {
        user_feature_list[user_info[i].first].push_back(user_info[i].second);
    }
    user_seg_str_list.push_back("");
    expandSegmentStr(user_seg_str_list, user_feature_list);
}

double AdSelector::getHistoryCTR(const std::vector<std::string>& all_fullkey)
{
    if (all_fullkey.empty())
        return 0;

    double max_ctr = 0;
    boost::unordered_map<std::string, double>::const_iterator it = history_ctr_data_.find(all_fullkey[0]);


    if (it != history_ctr_data_.end())
        max_ctr = it->second;
    else
        LOG(INFO) << "history ctr key not found: " << all_fullkey[0];

    if (all_fullkey.size() > 1)
    {
        //LOG(INFO) << "multi values : " << all_fullkey.size(); 
        for (size_t i = 1; i < all_fullkey.size(); ++i)
        {
            it = history_ctr_data_.find(all_fullkey[i]);
            if (it != history_ctr_data_.end())
            {
                max_ctr = std::max(it->second, max_ctr);
                //LOG(INFO) << "values : " << all_fullkey[i]; 
            }
            else
            {
                LOG(INFO) << "multi value not found: " << all_fullkey[i];
            }
        }
    }
    //LOG(INFO) << "history ctr key: " << all_fullkey[0] << ", ctr: " << max_ctr;
    return max_ctr;
}

void AdSelector::selectByRandSelectPolicy(std::size_t max_unclicked_retnum, std::vector<docid_t>& unclicked_doclist)
{
    std::set<int> rand_index_list;
    for(size_t i = 0; i < max_unclicked_retnum; ++i)
    {
        rand_index_list.insert(random_gen_());
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
    std::vector<docid_t>& recommended_doclist,
    std::vector<double>& score_list
    )
{
    // select the ads from recommend system if no any search results.
    return false;
}

// the ad feature should be full of all possible kinds of feature,
// this can be done while indexing the ad data.
bool AdSelector::select(const FeatureT& user_info,
    std::size_t max_select,
    std::vector<docid_t>& ad_doclist,
    std::vector<double>& score_list,
    PropSharedLockSet& propSharedLockSet)
{
    //if (ad_feature_list.size() != ad_doclist.size())
    //{
    //    LOG(ERROR) << "ad features not match doclist.";
    //    return false;
    //}
    if (ad_doclist.empty())
    {
        return selectFromRecommend(user_info, max_select, ad_doclist, score_list);
    }

    std::vector<std::string> user_seg_str;
    getUserSegmentStr(user_seg_str, user_info);

    std::size_t max_clicked_retnum = max_select/3 + 1;
    std::size_t max_unclicked_retnum = max_select- max_clicked_retnum;

    //std::map<docid_t, const FeatureMapT*> clicked_docfeature_list;
    std::vector<docid_t> unclicked_doclist;
    unclicked_doclist.reserve(ad_doclist.size());

    std::size_t max_tmp_clicked_num = max_clicked_retnum * 2;
    ScoreSortedHitQueue tmp_clicked_scorelist(max_tmp_clicked_num);

    for(size_t i = 0; i < ad_doclist.size(); ++i)
    {
        if (clicked_ads_.test(ad_doclist[i]))
        {
            // for clicked item, we filter the result first by the history ctr.
            //clicked_docfeature_list[ad_doclist[i]] = &(ad_feature_list[i]);
            double score = 0;
            if (ad_doclist.size() > max_tmp_clicked_num)
            {
                std::vector<std::string> all_fullkey = user_seg_str;
                expandSegmentStr(all_fullkey, ad_segid_data_[i]);
                score = getHistoryCTR(all_fullkey);
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
    FeatureT ad_features;

    std::vector<faceted::PropValueTable*> pvt_list;
    const FeatureMapT& ad_full_features = default_full_features_[AdSeg];
    for (FeatureMapT::const_iterator feature_it = ad_full_features.begin();
        feature_it != ad_full_features.end(); ++feature_it)
    {
        faceted::PropValueTable* pvt = NULL;
        if (groupManager_)
            groupManager_->getPropValueTable(feature_it->first);

        if (pvt)
            propSharedLockSet.insertSharedLock(pvt);
        pvt_list.push_back(pvt);
    }

    std::vector<std::string> value_list;
    for(int i = scoresize - 1; i >= 0; --i)
    {
        ScoreDoc item;
        item.docId = tmp_clicked_scorelist.pop().docId;

        ad_features.clear();
        faceted::PropValueTable::PropIdList propids;
        std::size_t feature_index = 0;
        for(FeatureMapT::const_iterator ad_it = ad_full_features.begin(); ad_it != ad_full_features.end(); ++ad_it)
        {
            if (pvt_list[feature_index])
            {
                pvt_list[feature_index]->getPropIdList(item.docId, propids);
                getValueStrFromPropId(pvt_list[feature_index], propids, value_list);
                for(std::size_t j = 0; j < value_list.size(); ++j)
                {
                    ad_features.push_back(std::make_pair(ad_it->first, value_list[j]));
                }
            }
            ++feature_index;
        }
        item.score = ad_click_predictor_->predict(user_info, ad_features);
        clicked_scorelist.insert(item);
    }
    // random select some un-clicked ads.
    if (max_unclicked_retnum < unclicked_doclist.size())
    {
        selectByRandSelectPolicy(max_unclicked_retnum, unclicked_doclist);
    }
    scoresize = clicked_scorelist.size();
    score_list.resize(ad_doclist.size());
    std::size_t ret_i = 0;
    double min_score = 0;
    for(int i = scoresize - 1; i >= 0; --i)
    {
        const ScoreDoc& item = clicked_scorelist.pop();
        ad_doclist[i] = item.docId;
        score_list[i] = item.score;
        if (item.score < min_score)
            min_score = item.score;
        ++ret_i;
    }
    for(size_t i = 0; i < unclicked_doclist.size(); ++i)
    {
        ad_doclist[ret_i] = unclicked_doclist[i];
        score_list[ret_i] = min_score - 1;
        ++ret_i;
    }
    ad_doclist.resize(max_select);
    score_list.resize(ad_doclist.size());
    return !ad_doclist.empty();
}

bool AdSelector::selectForTest(const FeatureT& user_info,
    std::size_t max_select,
    std::vector<docid_t>& ad_doclist,
    std::vector<double>& score_list)
{
    PropSharedLockSet propSharedLockSet;
    return select(user_info, max_select, ad_doclist, score_list, propSharedLockSet);
}

}

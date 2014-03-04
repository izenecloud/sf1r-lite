#include "AdRecommender.h"
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <3rdparty/am/google/sparsetable.h>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>

#define LATENT_VEC_DIM  10

namespace bfs = boost::filesystem;
using namespace boost::numeric::ublas;

namespace sf1r
{
static const double SMOOTH = 0.02;
static const double MAX_NORM = 100;

static double affinity(const AdRecommender::LatentVecT& left, const AdRecommender::LatentVecT& right)
{
    assert(left.size() == right.size());
    double sum = 0;
    for (size_t i = 0; i < left.size(); ++i)
    {
        sum += left[i] * right[i];
    }
    return sum;
}

static int getunviewed(const std::bitset<AdRecommender::MAX_AD_ITEMS>& bits, std::size_t index)
{
    if (index >= bits.count())
        return -1;
    for(std::size_t i = 0; i < bits.size(); ++i)
    {
        if (bits.test(i))
        {
            if (index == 0)
            {
                return i;
            }
            --index;
        }
    }
    return -1;
}

AdRecommender::AdRecommender()
    :use_ad_feature_(true)
{
}

AdRecommender::~AdRecommender()
{
}

void AdRecommender::init(const std::string& data_path, bool use_ad_feature)
{
    clicked_num_ = 1;
    impression_num_ = 1000;
    data_path_ = data_path;
    use_ad_feature_ = use_ad_feature;
    bfs::create_directories(data_path_);
    load();
    if (clicked_num_ < 1)
        clicked_num_ = 1;
    if (impression_num_ < clicked_num_)
        impression_num_ = clicked_num_*100;

    learning_rate_ = 1/(double)clicked_num_;
    ratio_ = -1 * (double)clicked_num_ /(double)(impression_num_ - clicked_num_);
    LOG(INFO) << "init clicked_num_: " << clicked_num_ << ", ratio_:" << ratio_;
    default_latent_.resize(LATENT_VEC_DIM, 1);
    //db_ = new MatrixType(10000, data_path + "/rec.db");
}

void AdRecommender::load()
{
    std::ifstream ifs(std::string(data_path_ + "/ad_latent.data").c_str());
    std::string data;
    if (ifs.good())
    {
        std::size_t len = 0;
        ifs.read((char*)&len, sizeof(len));
        data.resize(len);
        ifs.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<LatentVecContainerT> izd(data.data(), data.size());
        izd.read_image(ad_latent_vec_list_);
    }
    LOG(INFO) << "ad latent vec list loaded: " << ad_latent_vec_list_.size();
    ifs.close();

    ifs.open(std::string(data_path_ + "/user_latent.data").c_str());
    if (ifs.good())
    {
        std::size_t len = 0;
        ifs.read((char*)&len, sizeof(len));
        data.resize(len);
        ifs.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<LatentVecContainerT> izd(data.data(), data.size());
        izd.read_image(user_feature_latent_vec_list_);
    }
    LOG(INFO) << "user latent vec list loaded: " << user_feature_latent_vec_list_.size();
    ifs.close();

    ifs.open(std::string(data_path_ + "/history_data.data").c_str());
    if (ifs.good())
    {
        ifs.read((char*)&clicked_num_, sizeof(clicked_num_));
        ifs.read((char*)&impression_num_, sizeof(impression_num_));
    }
    LOG(INFO) << "history data: clicked " << clicked_num_ << ", impression " << impression_num_;
    ifs.close();

    ifs.open(std::string(data_path_ + "/ad_feature_info.data").c_str());
    if(ifs.good())
    {
        std::size_t len = 0;
        ifs.read((char*)&len, sizeof(len));
        data.resize(len);
        ifs.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<std::vector<std::string> > izd(data.data(), data.size());
        izd.read_image(ad_feature_value_list_);
        len = 0;
        ifs.read((char*)&len, sizeof(len));
        data.resize(len);
        ifs.read((char*)&data[0], len);
        izenelib::util::izene_deserialization<AdFeatureContainerT> izd_2(data.data(), data.size());
        izd_2.read_image(ad_features_map_);
        for(size_t i = 0; i < ad_feature_value_list_.size(); ++i)
        {
            ad_feature_value_id_list_[ad_feature_value_list_[i]] = i;
        }
    }
    LOG(INFO) << "ad feature items loaded: " << ad_feature_value_list_.size();
}

void AdRecommender::save()
{
    std::ofstream ofs(std::string(data_path_ + "/ad_latent.data").c_str());
    std::size_t len = 0;
    char* buf = NULL;
    {
        izenelib::util::izene_serialization<LatentVecContainerT> izs(ad_latent_vec_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
        ofs.close();
    }

    {
        ofs.open(std::string(data_path_ + "/user_latent.data").c_str());
        len = 0;
        izenelib::util::izene_serialization<LatentVecContainerT> izs(user_feature_latent_vec_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
        ofs.close();
    }

    {
        ofs.open(std::string(data_path_ + "/history_data.data").c_str());
        ofs.write((const char*)&clicked_num_, sizeof(clicked_num_));
        ofs.write((const char*)&impression_num_, sizeof(impression_num_));
        ofs.flush();
        ofs.close();
    }

    ofs.open(std::string(data_path_ + "/ad_feature_info.data").c_str());
    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<std::string> > izs(ad_feature_value_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
    }

    {
        len = 0;
        izenelib::util::izene_serialization<AdFeatureContainerT> izs(ad_features_map_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
    }
    ofs.flush();
    ofs.close();
}

//void AdRecommender::setMaxAdDocId(docid_t max_docid)
//{
//    ad_latent_vec_list_.resize(max_docid);
//}

void AdRecommender::getUserLatentVecKeys(const FeatureT& user_info, std::vector<std::string>& user_latentvec_keys)
{
    user_latentvec_keys.resize(user_info.size());
    std::size_t i = 0;
    for(FeatureT::const_iterator it = user_info.begin(); it != user_info.end(); ++it)
    {
        user_latentvec_keys[i++] = it->first + it->second;
    }
}

void AdRecommender::updateAdFeatures(const std::string& ad_docid, const std::vector<std::string>& features)
{
    boost::unique_lock<boost::shared_mutex> lock(ad_feature_lock_);
    std::set<uint32_t>& ad_feature_id_list = ad_features_map_[ad_docid];
    ad_feature_id_list.clear();
    for (std::size_t i = 0; i < features.size(); ++i)
    {
        uint32_t id = 0;
        boost::unordered_map<std::string, uint32_t>::iterator it = ad_feature_value_id_list_.find(features[i]);
        if (it == ad_feature_value_id_list_.end())
        {
            id = ad_feature_value_list_.size();
            ad_feature_value_list_.push_back(features[i]);
            ad_feature_value_id_list_[features[i]] = id;
            unviewed_items_.set(id);
        }
        else
        {
            id = it->second;
        }
        ad_feature_id_list.insert(id);
    }
}

void AdRecommender::getAdLatentVecKeys(const std::string& ad_docid, std::vector<std::string>& ad_latentvec_keys)
{
    if (!use_ad_feature_)
    {
        ad_latentvec_keys.resize(1);
        ad_latentvec_keys[0] = ad_docid;
        return;
    }
    AdFeatureContainerT::const_iterator it = ad_features_map_.find(ad_docid);
    if (it != ad_features_map_.end())
    {
        ad_latentvec_keys.resize(it->second.size());
        std::size_t i = 0;
        std::set<uint32_t>::const_iterator value_it = it->second.begin();
        for(; value_it != it->second.end(); ++value_it)
        {
            ad_latentvec_keys[i++] = ad_feature_value_list_[*value_it];
        }
    }
}

void AdRecommender::getCombinedUserLatentVec(const std::vector<std::string>& latentvec_keys, LatentVecT& latent_vec)
{
    latent_vec.clear();
    latent_vec.resize(default_latent_.size(), 0);
    for (std::size_t i = 0; i < latentvec_keys.size(); ++i)
    {
        LatentVecContainerT::const_iterator it = user_feature_latent_vec_list_.find(latentvec_keys[i]);
        if (it == user_feature_latent_vec_list_.end())
            continue;
        for(std::size_t j = 0; j < latent_vec.size(); ++j)
        {
            latent_vec[j] += it->second[j];
        }
    }
    for(std::size_t j = 0; j < latent_vec.size(); ++j)
    {
        if (latentvec_keys.size() == 0)
            latent_vec[j] = 1;
        else
            latent_vec[j] /= latentvec_keys.size();
    }
}

// get the user latent by combined the features of the user.
// Linear combined or non-linear combined both work. Some weighted can be considered.
void AdRecommender::getCombinedUserLatentVec(const std::vector<LatentVecT*>& latentvec_list, LatentVecT& latent_vec)
{
    latent_vec.clear();
    latent_vec.resize(default_latent_.size(), 0);
    for (std::size_t i = 0; i < latentvec_list.size(); ++i)
    {
        for(std::size_t j = 0; j < latent_vec.size(); ++j)
        {
            latent_vec[j] += (*latentvec_list[i])[j];
        }
    }
    for(std::size_t j = 0; j < latent_vec.size(); ++j)
    {
        if (latentvec_list.size() == 0)
            latent_vec[j] = 1;
        else
            latent_vec[j] /= latentvec_list.size();
    }
}

void AdRecommender::doRecommend(const std::string& user_str_id,
    const FeatureT& user_info, std::size_t max_return,
    std::vector<std::string>& recommended_items,
    std::vector<double>& score_list, bool rec_for_unview)
{
    std::vector<std::string> user_keys;
    getUserLatentVecKeys(user_info, user_keys);
    LatentVecT user_latent_vec;

    {
        boost::shared_lock<boost::shared_mutex> lock(user_latent_lock_);
        getCombinedUserLatentVec(user_keys, user_latent_vec);
    }
    
    bool has_unviewed_item = rec_for_unview && unviewed_items_.any();

    ScoreSortedAdQueue hit_list(max_return);
    {
        boost::shared_lock<boost::shared_mutex> lock(ad_latent_lock_);
        if (recommended_items.empty())
        {
            LatentVecContainerT::const_iterator it = ad_latent_vec_list_.begin();
            while (it != ad_latent_vec_list_.end())
            {
                if (!(it->second.empty()))
                {
                    ScoredAdItem item;
                    item.score = affinity(user_latent_vec, it->second);
                    item.key = it->first;
                    hit_list.insert(item);
                }
                ++it;
            }
        }
        else
        {
            for(size_t i = 0; i < recommended_items.size(); ++i)
            {
                LatentVecContainerT::const_iterator it = ad_latent_vec_list_.find(recommended_items[i]);
                if (it == ad_latent_vec_list_.end())
                    continue;
                if (it->second.empty())
                    continue;
                ScoredAdItem item;
                item.score = affinity(user_latent_vec, it->second);
                item.key = it->first;
                hit_list.insert(item);
            }
        }
    }
    double default_score = 0;
    if (has_unviewed_item)
        default_score = affinity(user_latent_vec, default_latent_);

    size_t scoresize = hit_list.size();
    recommended_items.resize(hit_list.size());
    score_list.resize(hit_list.size());
    for(int i = scoresize - 1; i >= 0; --i)
    {
        const ScoredAdItem& item = hit_list.pop();
        if (has_unviewed_item)
        {
            if (item.score < default_score)
            {
                boost::shared_lock<boost::shared_mutex> lock(ad_feature_lock_);
                // chose from unviewed items.
                int unview_index = getunviewed(unviewed_items_, scoresize - 1 - i);
                if (unview_index != -1 && unview_index < (int)ad_feature_value_list_.size())
                {
                    recommended_items[i] = ad_feature_value_list_[unview_index];
                    score_list[i] = default_score;
                    LOG(INFO) << "recommend from unviewed items : " << recommended_items[i];
                    continue;
                }
            }
        }
        recommended_items[i] = item.key;
        score_list[i] = item.score;
    }
}

void AdRecommender::recommendFromCand(const std::string& user_str_id,
    const FeatureT& user_info, std::size_t max_return,
    std::vector<std::string>& recommended_items,
    std::vector<double>& score_list)
{
    doRecommend(user_str_id, user_info, max_return, recommended_items, score_list, false);
}

void AdRecommender::recommend(const std::string& user_str_id,
    const FeatureT& user_info, std::size_t max_return,
    std::vector<std::string>& recommended_items,
    std::vector<double>& score_list)
{
    //LOG(INFO) << "begin do the recommend for user : " << user_str_id;
    doRecommend(user_str_id, user_info, max_return, recommended_items, score_list, true);
    //LOG(INFO) << "finished the ad recommend, size: " << recommended_items.size();
}

void AdRecommender::update(const std::string& user_str_id,
    const FeatureT& user_info, const std::string& ad_docid, bool is_clicked)
{
    impression_num_++;
    if (is_clicked)
    {
        clicked_num_++;
    }
    if (impression_num_ % 1000 == 0)
    {
        learning_rate_ = 1/(double)clicked_num_;
        ratio_ = -1*SMOOTH*(double)clicked_num_/(double)(impression_num_ - clicked_num_) + (1 - SMOOTH)*ratio_;
        //LOG(INFO) << " impression_num_ : " << impression_num_ << ", clicked_num_: " << clicked_num_
        //    << ", ratio_ : " << ratio_;
    }

    std::vector<std::string> user_keys;
    getUserLatentVecKeys(user_info, user_keys);
    std::vector<LatentVecT*> user_feature_latent_list;

    boost::unique_lock<boost::shared_mutex> lock_user(user_latent_lock_);
    for (size_t i = 0; i < user_keys.size(); ++i)
    {
        std::pair<LatentVecContainerT::iterator, bool> it_pair = user_feature_latent_vec_list_.insert(std::make_pair(user_keys[i], default_latent_));
        if (it_pair.second && !is_clicked)
        {
            // a new feature value
            continue;
        }
        LatentVecT& feature_latent = it_pair.first->second;
        user_feature_latent_list.push_back(&feature_latent);
    }

    std::vector<std::string> ad_keys;
    std::vector<LatentVecT*> ad_feature_latent_list;

    boost::unique_lock<boost::shared_mutex> lock_ad(ad_latent_lock_);
    {
        boost::shared_lock<boost::shared_mutex> lock_feature(ad_feature_lock_);
        getAdLatentVecKeys(ad_docid, ad_keys);
        for(size_t i = 0; i < ad_keys.size(); ++i)
        {
            std::pair<LatentVecContainerT::iterator, bool> it_pair = ad_latent_vec_list_.insert(std::make_pair(ad_keys[i], default_latent_));
            ad_feature_latent_list.push_back(&(it_pair.first->second));
            unviewed_items_.reset(ad_feature_value_id_list_[ad_keys[i]]);
        }
    }

    double gradient = learning_rate_;
    double new_max_norm = 0;
    if (!is_clicked)
        gradient = ratio_ * learning_rate_;
    for (size_t k = 0; k < ad_feature_latent_list.size(); ++k)
    {
        LatentVecT& ad_latent = *(ad_feature_latent_list[k]);
        LatentVecT combined_user_latent;
        getCombinedUserLatentVec(user_feature_latent_list, combined_user_latent);
        for (size_t i = 0; i < ad_latent.size(); ++i)
        {
            ad_latent[i] += gradient * combined_user_latent[i];
            new_max_norm = std::max(new_max_norm, std::fabs(ad_latent[i]));
            for (size_t j = 0; j < user_feature_latent_list.size(); ++j)
            {
                //(*(user_feature_latent_list[j]))[i] += gradient*ad_latent[i]*combined_latent[i]/(*(user_feature_latent_list[j]))[i];
                (*(user_feature_latent_list[j]))[i] += gradient*ad_latent[i];
                new_max_norm = std::max(new_max_norm, std::fabs((*(user_feature_latent_list[j]))[i]));
            }
        }
    }
    if (new_max_norm > MAX_NORM)
    {
        // scale the vectors to obey the norm constrain.
        double scale = MAX_NORM/new_max_norm;
        for (LatentVecContainerT::iterator it = ad_latent_vec_list_.begin();
            it != ad_latent_vec_list_.end(); ++it)
        {
            for(size_t j = 0; j < it->second.size(); ++j)
            {
                it->second[j] *= scale;
            }
        }
        for (LatentVecContainerT::iterator it = user_feature_latent_vec_list_.begin();
            it != user_feature_latent_vec_list_.end(); ++it)
        {
            for (size_t j = 0; j < it->second.size(); ++j)
            {
                it->second[j] *= scale; 
            }
        }
    }
    if (is_clicked)
    {
        //LOG(INFO) << "clicked ad : " << ad_docid << ", user: " << user_str_id;
    }
}

void AdRecommender::dumpUserLatent()
{
    LOG(INFO) << "ad feature value size: " << ad_feature_value_list_.size()
        << ", " << ad_feature_value_id_list_.size() << ", " << ad_latent_vec_list_.size();
    LOG(INFO) << "currently unviewed_items : " << unviewed_items_.count();
    std::ofstream ofs(std::string(data_path_ + "/dumped_userlatent.txt").c_str());
    for (LatentVecContainerT::const_iterator it = user_feature_latent_vec_list_.begin();
        it != user_feature_latent_vec_list_.end(); ++it)
    {
        ofs << "key: " << it->first << ", latent vector: ";
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            ofs << it->second[i] << ",";
        }
        ofs << std::endl;
    }
    ofs.flush();
}

}


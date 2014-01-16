#include "AdRecommender.h"
#include <search-manager/HitQueue.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <3rdparty/am/google/sparsetable.h>

#define LATENT_VEC_DIM  10

namespace bfs = boost::filesystem;
using namespace boost::numeric::ublas;

namespace sf1r
{
static const double SMOOTH = 0.02;

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

AdRecommender::AdRecommender()
    :db_(NULL)
{
}

AdRecommender::~AdRecommender()
{
    if (db_)
        delete db_;
}

void AdRecommender::init(const std::string& data_path)
{
    clicked_num_ = 1;
    impression_num_ = 1000;
    data_path_ = data_path;
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
    db_ = new MatrixType(10000, data_path + "/rec.db");
}

void AdRecommender::load()
{
}

void AdRecommender::save()
{
}

void AdRecommender::setMaxAdDocId(docid_t max_docid)
{
    ad_latent_vec_list_.resize(max_docid);
}

void AdRecommender::getUserLatentVecKeys(const FeatureT& user_info, std::vector<std::string>& user_latentvec_keys)
{
    user_latentvec_keys.resize(user_info.size());
    std::size_t i = 0;
    for(FeatureT::const_iterator it = user_info.begin(); it != user_info.end(); ++it)
    {
        user_latentvec_keys[i++] = it->first + it->second;
    }
}

void AdRecommender::getCombinedUserLatentVec(const std::vector<std::string>& latentvec_keys, LatentVecT& latent_vec)
{
    latent_vec.clear();
    latent_vec.resize(default_latent_.size(), 0);
    for (std::size_t i = 0; i < latentvec_keys.size(); ++i)
    {
        LatentVecContainerT::const_iterator it = feature_latent_vec_list_.find(latentvec_keys[i]);
        if (it == feature_latent_vec_list_.end())
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

void AdRecommender::recommendFromCand(const std::string& user_str_id,
    const FeatureT& user_info, std::size_t max_return,
    std::vector<docid_t>& recommended_doclist,
    std::vector<double>& score_list)
{
}

void AdRecommender::recommend(const std::string& user_str_id,
    const FeatureT& user_info, std::size_t max_return,
    std::vector<docid_t>& recommended_doclist,
    std::vector<double>& score_list)
{
    LOG(INFO) << "begin do the recommend.";
    boost::shared_ptr<const RowType> row = db_->row(0);
    std::vector<std::string> user_keys;
    getUserLatentVecKeys(user_info, user_keys);
    LatentVecT user_latent_vec;
    getCombinedUserLatentVec(user_keys, user_latent_vec);
    
    ScoreSortedHitQueue hit_list(max_return);
    AdLatentVecContainerT::const_iterator it = ad_latent_vec_list_.begin();
    docid_t i = 0;
    while (it != ad_latent_vec_list_.end())
    {
        if (!(*it).empty())
        {
            ScoreDoc item;
            item.score = affinity(user_latent_vec, *it);
            item.docId = i;
            hit_list.insert(item);
        }
        ++it;
        ++i;
    }
    size_t scoresize = hit_list.size();
    recommended_doclist.resize(hit_list.size());
    score_list.resize(hit_list.size());
    for(int i = scoresize - 1; i >= 0; --i)
    {
        const ScoreDoc& item = hit_list.pop();
        recommended_doclist[i] = item.docId;
        score_list[i] = item.score;
    }

    LOG(INFO) << "finished the ad recommend, size: " << recommended_doclist.size();
}

void AdRecommender::update(const std::string& user_str_id,
    const FeatureT& user_info, docid_t ad_docid, bool is_clicked)
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
    LatentVecT& ad_latent = ad_latent_vec_list_[ad_docid];
    if (ad_latent.empty())
    {
        ad_latent = default_latent_;
    }

    std::vector<std::string> user_keys;
    getUserLatentVecKeys(user_info, user_keys);
    std::vector<LatentVecT*> user_feature_latent_list;
    for (size_t i = 0; i < user_keys.size(); ++i)
    {
        std::pair<LatentVecContainerT::iterator, bool> it_pair = feature_latent_vec_list_.insert(std::make_pair(user_keys[i], default_latent_));
        if (it_pair.second && !is_clicked)
        {
            // a new feature value
            continue;
        }
        LatentVecT& feature_latent = it_pair.first->second;
        user_feature_latent_list.push_back(&feature_latent);
    }
    LatentVecT combined_latent;
    getCombinedUserLatentVec(user_feature_latent_list, combined_latent);
    double gradient = learning_rate_;
    if (!is_clicked)
        gradient = ratio_ * learning_rate_;
    for (size_t i = 0; i < ad_latent.size(); ++i)
    {
        ad_latent[i] += gradient * combined_latent[i];
        for (size_t j = 0; j < user_feature_latent_list.size(); ++j)
        {
            //(*(user_feature_latent_list[j]))[i] += gradient*ad_latent[i]*combined_latent[i]/(*(user_feature_latent_list[j]))[i];
            (*(user_feature_latent_list[j]))[i] += gradient*ad_latent[i];
        }
    }
    if (is_clicked)
    {
        //LOG(INFO) << "clicked ad : " << ad_docid << ", user: " << user_str_id;
    }
}

void AdRecommender::dumpUserLatent()
{
    std::ofstream ofs(std::string(data_path_ + "/dumped_userlatent.txt").c_str());
    for (LatentVecContainerT::const_iterator it = feature_latent_vec_list_.begin();
        it != feature_latent_vec_list_.end(); ++it)
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


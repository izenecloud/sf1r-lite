#ifndef SF1_AD_RECOMMENDER_H_
#define SF1_AD_RECOMMENDER_H_

#include <util/singleton.h>
#include <common/type_defs.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <am/matrix/matrix_db.h>
#include <vector>
#include <boost/unordered_map.hpp>
#include <util/PriorityQueue.h>

namespace sf1r
{

struct ScoredAdItem
{
    std::string key;
    double score;
};

class ScoreSortedAdQueue
{
    class Queue_ : public izenelib::util::PriorityQueue<ScoredAdItem>
    {
    public:
        Queue_(size_t size)
        {
            initialize(size);
        }
    protected:
        bool lessThan(const ScoredAdItem& o1, const ScoredAdItem& o2) const
        {
            if (std::fabs(o1.score - o2.score) < std::numeric_limits<score_t>::epsilon())
            {
                return o1.key < o2.key;
            }
            return (o1.score < o2.score);
        }
    };

public:
    ScoreSortedAdQueue(size_t size) : queue_(size)
    {
    }

    ~ScoreSortedAdQueue() {}

    bool insert(const ScoredAdItem& doc)
    {
        return queue_.insert(doc);
    }

    ScoredAdItem pop()
    {
        return queue_.pop();
    }
    const ScoredAdItem& top()
    {
        return queue_.top();
    }
    ScoredAdItem& operator[](size_t pos)
    {
        return queue_[pos];
    }
    ScoredAdItem& getAt(size_t pos)
    {
        return queue_.getAt(pos);
    }
    size_t size()
    {
        return queue_.size();
    }
    void clear() {}

private:
    Queue_ queue_;

};

class AdRecommender
{
public:
    static const int MAX_AD_ITEMS = 1024*1024;
    typedef std::vector<double> LatentVecT;
    typedef std::vector<std::pair<std::string, std::string> > FeatureT;

    AdRecommender();
    ~AdRecommender();

    void init(const std::string& data_path, bool use_ad_feature);

    void recommend(const std::string& user_str_id,
        const FeatureT& user_info, std::size_t max_return,
        std::vector<std::string>& recommended_items,
        std::vector<double>& score_list);
    void recommendFromCand(const std::string& user_str_id,
        const FeatureT& user_info, std::size_t max_return,
        std::vector<std::string>& recommended_doclist,
        std::vector<double>& score_list);

    void update(const std::string& user_str_id, const FeatureT& user_info,
        const std::string& ad_docid, bool is_clicked);

    void load();
    void save();
    void dumpUserLatent();
    void deleteAdDoc(const std::string& docid);
    //void setMaxAdDocId(docid_t max_docid);
    void updateAdFeatures(const std::string& ad_docid, const std::vector<std::string>& features);

private:
    typedef izenelib::am::MatrixDB<uint32_t, double> MatrixType;
    typedef MatrixType::row_type RowType;
    typedef boost::unordered_map<std::string, LatentVecT> LatentVecContainerT;
    //typedef std::vector<LatentVecT> AdLatentVecContainerT;
    
    typedef std::map<std::string, std::set<uint32_t> >  AdFeatureContainerT;

    void getAdLatentVecKeys(const std::string& ad_docid, std::vector<std::string>& ad_latentvec_keys);
    void getUserLatentVecKeys(const FeatureT& user_info, std::vector<std::string>& user_latentvec_keys);
    void getCombinedUserLatentVec(const std::vector<std::string>& latentvec_keys, LatentVecT& latent_vec);
    void getCombinedUserLatentVec(const std::vector<LatentVecT*>& latentvec_list, LatentVecT& latent_vec);

    MatrixType* db_;
    std::string data_path_;
    bool use_ad_feature_;
    boost::unordered_map<std::string, FeatureT> user_feature_map_;
    LatentVecContainerT ad_latent_vec_list_;
    LatentVecContainerT user_feature_latent_vec_list_;

    std::size_t clicked_num_;
    std::size_t impression_num_;
    double  ratio_;
    double  learning_rate_;
    LatentVecT default_latent_;
    // total clicked number and last clicked time to evaluate the popularity of ad.
    boost::unordered_map<std::string, std::pair<uint32_t, uint64_t> >  ad_clicked_data_;
    // the last activity time for user. remove dead user period.
    boost::unordered_map<std::string, uint64_t>  user_activity_list_;
    std::vector<std::string> ad_feature_value_list_;
    boost::unordered_map<std::string, uint32_t> ad_feature_value_id_list_;
    AdFeatureContainerT ad_features_map_;
    std::bitset<MAX_AD_ITEMS> unviewed_items_;
};

} //namespace sf1r

#endif

#ifndef SF1_AD_RECOMMENDER_H_
#define SF1_AD_RECOMMENDER_H_

#include <util/singleton.h>
#include <common/type_defs.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <am/matrix/matrix_db.h>
#include <vector>
#include <boost/unordered_map.hpp>

namespace sf1r
{

class AdRecommender
{
public:
    typedef std::vector<double> LatentVecT;
    typedef std::vector<std::pair<std::string, std::string> > FeatureT;

    static AdRecommender* get()
    {
        return izenelib::util::Singleton<AdRecommender>::get();
    }

    AdRecommender();
    ~AdRecommender();

    void init(const std::string& data_path);

    void recommend(const std::string& user_str_id,
        const FeatureT& user_info, std::size_t max_return,
        std::vector<docid_t>& recommended_doclist,
        std::vector<double>& score_list);

    void update(const std::string& user_str_id, const FeatureT& user_info,
        const std::string& ad_str_docid, bool is_clicked);

    void load();
    void save();

private:
    typedef izenelib::am::MatrixDB<uint32_t, double> MatrixType;
    typedef MatrixType::row_type RowType;
    typedef boost::unordered_map<std::string, LatentVecT> LatentVecContainerT;

    void getUserLatentVecKeys(const FeatureT& user_info, std::vector<std::string>& user_latentvec_keys);
    void getCombinedUserLatentVec(const std::vector<std::string>& latentvec_keys, LatentVecT& latent_vec);
    void getCombinedUserLatentVec(const std::vector<LatentVecT*>& latentvec_list, LatentVecT& latent_vec);

    MatrixType* db_;
    std::string data_path_;
    boost::unordered_map<std::string, FeatureT> user_feature_map_;
    LatentVecContainerT ad_latent_vec_list_;
    LatentVecContainerT feature_latent_vec_list_;

    std::size_t clicked_num_;
    std::size_t impression_num_;
    double  ratio_;
    double  learning_rate_;
    LatentVecT default_latent_;
};

} //namespace sf1r

#endif

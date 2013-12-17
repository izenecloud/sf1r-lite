#ifndef SF1_AD_SELECTOR_H_
#define SF1_AD_SELECTOR_H_

#include "AdClickPredictor.h"
#include <util/singleton.h>
#include <boost/lexical_cast.hpp>
#include <common/PropSharedLockSet.h>
#include <search-manager/NumericPropertyTableBuilder.h>
#include <boost/unordered_map.hpp>
#include <bitset>
#include <boost/thread.hpp>

namespace sf1r
{

class AdClickPredictor;
class AdSelector
{
public:
    static AdSelector* get()
    {
        return izenelib::util::Singleton<AdSelector>::get();
    }

    typedef std::vector<std::pair<std::string, std::string> > FeatureT;
    typedef std::map<std::string, std::vector<std::string> > FeatureMapT;
    AdSelector();
    ~AdSelector();

    void init(const std::string& segments_data_path, AdClickPredictor* pad_predictor);
    void stop();

    bool selectFromRecommend(const FeatureT& user_info, std::size_t max_return,
        std::vector<docid_t>& recommended_doclist);

    bool select(const FeatureT& user_info,
        const std::vector<FeatureT>& ad_feature_list, 
        std::size_t max_return,
        std::vector<docid_t>& ad_doclist);

    void updateClicked(docid_t ad_id);
    void updateSegments(const FeatureT& segments);
    void load();
    void save();

private:

    static const int MAX_AD_DOCID = 1024*1024*1024;

    void updateFunc();
    void selectByRandSelectPolicy(std::size_t max_unclicked_retnum, std::vector<docid_t>& unclicked_doclist);
    void computeHistoryCTR();
    double getHistoryCTR(const FeatureT& user, const FeatureT& ad);

    std::string segments_data_path_;
    AdClickPredictor* ad_click_predictor_;
    boost::unordered_map<std::string, double> history_ctr_data_;
    std::bitset<MAX_AD_DOCID>  clicked_ads_;
    FeatureMapT  all_segments_;
    FeatureMapT  updated_segments_;
    boost::mutex segment_mutex_;
    std::map<std::string, std::size_t> init_counter_;
    FeatureMapT default_full_features_;
    boost::thread  ctr_update_thread_;
};

} //namespace sf1r

#endif

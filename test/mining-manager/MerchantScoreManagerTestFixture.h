///
/// @file MerchantScoreManagerTestFixture.h
/// @brief fixture class to test MerchantScoreManager functions.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-14
///

#ifndef SF1R_MERCHANT_SCORE_MANAGER_TEST_FIXTURE_H
#define SF1R_MERCHANT_SCORE_MANAGER_TEST_FIXTURE_H

#include <mining-manager/merchant-score-manager/MerchantScoreManager.h>
#include <mining-manager/group-manager/PropValueTable.h>

#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class MerchantScoreManagerTestFixture
{
public:
    MerchantScoreManagerTestFixture();

    void resetInstance();

    void checkEmptyScore();

    void setScore1();
    void checkScore1();

    void setScore2();
    void checkScore2();

private:
    merchant_id_t getMerchantId_(const std::string& merchant) const;
    category_id_t getCategoryId_(const std::string& category) const;

    void checkNonExistingId_();

    void checkAllScore1_();
    void checkPartScore1_();
    void checkIdScore1_();

    void checkAllScore2_();
    void checkPartScore2_();
    void checkIdScore2_();

private:
    std::string groupDir_;
    std::string scoreFile_;

    boost::scoped_ptr<faceted::PropValueTable> merchantValueTable_;
    boost::scoped_ptr<faceted::PropValueTable> categoryValueTable_;

    boost::scoped_ptr<MerchantScoreManager> merchantScoreManager_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORE_MANAGER_TEST_FIXTURE_H

/**
 * @file TitlePCAWrapper.h
 * @brief a wrapper class for ilplib::knlp functions.
 */

#ifndef SF1R_TITLE_PCA_WRAPPER_H
#define SF1R_TITLE_PCA_WRAPPER_H

#include <vector>
#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>

namespace ilplib
{
namespace knlp
{
    class TitlePCA;
}
}

namespace sf1r
{

class TitlePCAWrapper
{
public:

    static TitlePCAWrapper* get();

    TitlePCAWrapper();
    ~TitlePCAWrapper();

    bool loadDictFiles(const std::string& dictDir);
    void pca(const std::string& line,
         std::vector<std::pair<std::string, float> >& tks,
         std::string& brand,
         std::string& model_type,
         std::vector<std::pair<std::string, float> >& sub_tks, bool do_sub)const;

private:
    std::string dictDir_;
    bool isDictLoaded_;
    boost::scoped_ptr<ilplib::knlp::TitlePCA> title_pca_;
};

} // namespace sf1r

#endif // SF1R_TITLE_PCA_WRAPPER_H

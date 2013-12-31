/**
 * @file ProductForwardManager.h
 * @brief a list which stores a product tokenizer scores for each doc.
 */

#ifndef SF1R_PRODUCT_FORWARD_MANAGER_H
#define SF1R_PRODUCT_FORWARD_MANAGER_H

#include <common/inttypes.h>
#include <common/type_defs.h>
#include "../suffix-match-manager/ProductTokenizer.h"
#include <string>
#include <vector>

namespace sf1r
{
using izenelib::util::KString; 
class DocumentManager;
class ProductForwardManager
{
public:
    ProductForwardManager(
        const std::string& dirPath,
        const std::string& propName,
        ProductTokenizer* tokenizer,
        bool isDebug = 0);

    const std::string& dirPath() const { return dirPath_; }
    const std::string& propName() const { return propName_; }

    bool open();
    
    void resize(unsigned int size);
    bool insert(std::vector<std::string>& tmp_index);
    const docid_t getLastDocId() const { return lastDocid_; }

    std::string getIndex(unsigned int index);
    bool save(unsigned int last_doc);
    bool load();
    void clear();
    void copy(std::vector<std::string>& index);
    void forwardSearch(const std::string& src, const std::vector<std::pair<double, docid_t> >& docs, std::vector<std::pair<double, docid_t> >& res);

private:
    double compare_(const uint32_t q_brand, const uint32_t q_model, const std::vector<uint32_t>& q_res, const double q_score, const docid_t docid);
    static bool cmp_(const std::pair<double, docid_t>& x, const std::pair<double, docid_t>& y);

    const std::string dirPath_;

    const std::string propName_;

    std::vector<std::string> forward_index_;

    docid_t lastDocid_;

    bool isDebug_;

    ProductTokenizer* tokenizer_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ReadLock;
    typedef boost::unique_lock<MutexType> WriteLock;

    mutable MutexType mutex_;
};

} // namespace sf1r

#endif // SF1R_PRODUCT_FORWARD

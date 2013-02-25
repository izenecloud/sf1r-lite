#ifndef SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_
#define SF1R_MINING_INCREMENTAL_FUZZY_MANAGER_

#include "IncrementalFuzzyIndex.h" 

namespace sf1r
{
class IncrementalFuzzyManager
{
public:
    IncrementalFuzzyManager(const std::string& path,
                       const std::string& tokenize_path,
                       const std::string& property,
                       boost::shared_ptr<DocumentManager>& document_manager,
                       boost::shared_ptr<IDManager>& idManager,
                       boost::shared_ptr<LAManager>& laManager,
                       IndexBundleSchema& indexSchema
                      );

    ~IncrementalFuzzyManager();

    void startIncrementalManager();

    bool initMainFuzzyIndexBarrel_();
    bool init_tmpBerral();

    void InitManager_();

    void createIndex_();

    bool index_(uint32_t& docId, std::string propertyString);

    bool fuzzySearch_(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity);

    bool exactSearch_(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity);

    void doCreateIndex_();
    void mergeIndex();

    void buildTokenizeDic();
    void setLastDocid(uint32_t last_docid);
    void getLastDocid(uint32_t& last_docid);

    void getDocNum(uint32_t& docNum);
    void getMaxNum(uint32_t& maxNum);
    void prepare_index_();

    void reset();
    void save_();

    bool saveLastDocid_(std::string path = "");
    bool loadLastDocid_(std::string path = "");

    void delete_AllIndexFile();

private:
    uint32_t last_docid_;

    std::string index_path_;
    std::string tokenize_path_;
    std::string property_;

    unsigned int BarrelNum_;

    IncrementalFuzzyIndex* pMainFuzzyIndexBarrel_;
    IncrementalFuzzyIndex* pTmpFuzzyIndexBarrel_;

    unsigned int IndexedDocNum_;
    bool isMergingIndex_;
    bool isInitIndex_;
    bool isAddingIndex_;
    bool isStartFromLocal_;

    boost::shared_ptr<DocumentManager> document_manager_;

    boost::shared_ptr<IDManager> idManager_;

    boost::shared_ptr<LAManager> laManager_;

    IndexBundleSchema indexSchema_;

    cma::Analyzer* analyzer_;
    cma::Knowledge* knowledge_;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;
    mutable MutexType mutex_;
};

}
#endif
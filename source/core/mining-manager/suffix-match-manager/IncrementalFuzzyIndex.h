#ifndef SF1R_MINING_INCREMENTAL_FUZZY_INDEX_
#define SF1R_MINING_INCREMENTAL_FUZZY_INDEX_

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <stdio.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <ir/id_manager/IDManager.h>
#include <configuration-manager/PropertyConfig.h>
#include <util/ClockTimer.h>
#include <boost/filesystem.hpp>

namespace cma
{
class Analyzer;
class Knowledge;
}
/**
   @brief The search result from IncrementIndex just need two parts: vector<uint32_t> ResultList and vectort<similarity> ResultList;
*/
namespace sf1r
{

#define MAX_INCREMENT_DOC 500000
#define MAX_POS_LEN 24
#define MAX_HIT_NUMBER 3
#define MAX_SUB_LEN 15

class DocumentManager;
class LAManager;
using izenelib::ir::idmanager::IDManager;
namespace bfs = boost::filesystem;

/**
    @brife use this 24 bit data to mark if is this position is hit or not;
    postion 0-23;
*/
struct BitMap
{
    unsigned int data_;//init is zero

    unsigned int offset_;

    bool getBitMap(unsigned short i) const
    {
        if (i > MAX_POS_LEN)
        {
            return false;
        }
        unsigned int data = 0;
        data = data_>>i;
        unsigned int j = 1;
        return (data&j) == 1;
    }
    bool setBitMap(unsigned short i)
    {
        if (i > MAX_POS_LEN)
        {
            return false;
        }
        unsigned int data = 1;
        data<<=i;
        data_ |= data;
        return true;
    }

    void reset()
    {
        data_ = 0;
    }
};

/**
    @brief IndexItem for forwardIndex;
*/
struct IndexItem
{
    uint32_t Docid_;

    uint32_t Termid_;

    unsigned short Pos_;

    void setIndexItem_(uint32_t docid, uint32_t termid, unsigned short pos)
    {
        Docid_ = docid;
        Termid_ = termid;
        Pos_ = pos;
    }

    IndexItem()
    {
        Docid_ = 0;
        Termid_ = 0;
        Pos_ = 0;
    }

    IndexItem(const IndexItem& indexItem)
    {
        *this = indexItem;
    }
};

struct pairLess
{
    bool operator() (const pair<unsigned int, unsigned short>&s1, const pair<unsigned int, unsigned short>&s2) const
    {
        return s1.first < s2.first;
    }
};

/**
    @brief ForwardIndex is for restore query
*/
class ForwardIndex
{
public:
    ForwardIndex(std::string index_path, unsigned int Max_Doc_Num);
    ~ForwardIndex();

    bool checkLocalIndex_();

    void addDocForwardIndex_(uint32_t docId, std::vector<uint32_t>& termidList
                                , std::vector<unsigned short>& posList);
    void addOneIndexIterms_(uint32_t docId, uint32_t termid, unsigned short pos);
    void addIndexItem_(const IndexItem& indexItem);
    void setPosition_(std::vector<pair<uint32_t, unsigned short> >*& v);
    void resetBitMapMatrix();

    bool getTermIDByPos_(const uint32_t& docid
                        , const std::vector<unsigned short>& pos
                        , std::vector<uint32_t>& termidList);
    void getLongestSubString_(std::vector<uint32_t>& termidList
                        , const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList
                        , uint32_t& hitdoc);

    bool save_(std::string path = "");
    bool load_(std::string path = "");

    uint32_t getScore(std::vector<uint32_t> &v, uint32_t & docid);
    uint32_t getIndexCount_();
    std::vector<IndexItem>& getIndexItem_();
    std::string getPath();
    
    void print();
private:
    uint32_t includeNum(const vector<uint32_t>& vec1,const vector<uint32_t>& vec2);

private:
    BitMap* BitMapMatrix_;
    
    std::string index_path_;
    std::vector<IndexItem> IndexItems_;

    unsigned int IndexCount_;
    unsigned int Max_Doc_Num_;
    unsigned int start_docid_;
    unsigned int max_docid_;
};

class InvertedIndex
{
public:
    InvertedIndex(std::string file_path,
                   unsigned int Max_Doc_Num,
                   cma::Analyzer* &analyzer);
    ~InvertedIndex();

    bool checkLocalIndex_();
    void addIndex_(uint32_t docid, std::vector<uint32_t> termidList, std::vector<unsigned short>& posList);

    void getResultAND_(const std::vector<uint32_t>& termidList
                        , std::vector<uint32_t>& resultList
                        , std::vector<double>& ResultListSimilarity);

    bool getTermIdResult_(const uint32_t& termid
                            , std::vector<pair<uint32_t, unsigned short> >*& v
                            , uint32_t& size);

    void getTermIdPos_(const uint32_t& termid
                        , std::vector<pair<uint32_t, unsigned short> >*& v);

    void getResultORFuzzy_(const std::vector<uint32_t>& termidList
                            , std::vector<std::vector<pair<uint32_t, unsigned short> >* >& resultList);

    void print();

    bool save_(std::string path = "");
    bool load_(std::string path = "");

    void prepare_index_();
    void addTerm_(const pair<uint32_t, uint32_t> &termid, const std::vector<pair<uint32_t, unsigned short> >& docids);

    std::set<std::pair<uint32_t, uint32_t>, pairLess>& gettermidList_();
    std::vector<std::vector<pair<uint32_t, unsigned short> > >& getdocidLists_();
    const string& getPath() const;

private:
    int BinSearch_(std::vector<pair<unsigned int, unsigned short> >&A, int min, int max, unsigned int key);
    
    void mergeAnd_(const std::vector<std::vector<pair<uint32_t, unsigned short> >* >& docidLists
                    , std::vector<uint32_t>& resultList
                    , std::vector<double>& ResultListSimilarity);
private:
    std::string Increment_index_path_;
    std::vector<std::vector<pair<uint32_t, unsigned short> > > docidLists_;

    std::vector<unsigned int> docidNumbers_;
    std::set<std::pair<uint32_t, uint32_t>, pairLess> termidList_;

    unsigned int termidNum_;
    unsigned int allIndexDocNum_;
    unsigned int Max_Doc_Num_;
    cma::Analyzer* analyzer_;
};


class IncrementalFuzzyIndex
{
public:
    IncrementalFuzzyIndex(
        const std::string& path,
        boost::shared_ptr<IDManager>& idManager,
        boost::shared_ptr<LAManager>& laManager,
        IndexBundleSchema& indexSchema,
        unsigned int Max_Doc_Num,
        cma::Analyzer* &analyzer);

    ~IncrementalFuzzyIndex();

    void reset();

    bool init(std::string& path);
    uint32_t getScore(std::vector<uint32_t> &v, uint32_t & docid);

    void setStatus();
    void resetStatus();

    bool buildIndex_(uint32_t docId, std::string& text);
    
    bool score(const std::string& query
                    , std::vector<uint32_t>& resultList
                    , std::vector<double> &ResultListSimilarity);

    void getFuzzyResult_(std::vector<uint32_t>& termidList
                            , std::vector<uint32_t>& resultList
                            , std::vector<double>& ResultListSimilarity
                            , uint32_t& hitdoc);
    
    void getExactResult_(std::vector<uint32_t>& termidList
                            , std::vector<uint32_t>& resultList
                            , std::vector<double>& ResultListSimilarity);

    void print();
    void prepare_index_();

    ForwardIndex* getForwardIndex();
    InvertedIndex* getIncrementIndex();

    bool load_(string path = "");
    bool save_(string path = "");

private:
    std::string doc_file_path_;
    
    bool isAddedIndex_;
    unsigned int DocNumber_;
    
    ForwardIndex* pForwardIndex_;
    InvertedIndex* pIncrementIndex_;

    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<LAManager> laManager_;

    IndexBundleSchema& indexSchema_;
    unsigned int Max_Doc_Num_;
    cma::Analyzer* analyzer_;
};

}
#endif
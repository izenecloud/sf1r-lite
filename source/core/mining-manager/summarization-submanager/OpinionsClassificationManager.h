#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_CLASSFICATION_OPINIONMANAGER_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_CLASSFICATION_OPINIONMANAGER_H

#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <am/leveldb/Table.h>
#include <util/ustring/UString.h>
#include <icma/icma.h>
#include <string>
#include <iostream>

namespace sf1r
{

using namespace cma;
using namespace std;
using std::string;
using std::vector;
using izenelib::util::UString;

class OpinionsClassificationManager
{
    typedef izenelib::am::leveldb::Table<std::string, int> LevelDBType;
    LevelDBType* dbTable_;
    vector<pair<string,bool> > trainData_;
    vector<string> badData_;
    vector<string> goodData_;
    list<string> wordList_;
    vector<string> goodWord_;    //好
    vector<string> badWord_;     //差
    vector<string> reverseWord_; //不
    vector<string> reverseData_;
    vector<string> pairWord_;    //性价比
    vector<pair<string,string> > goodPair_;  //性价比 高
    vector<pair<string,string> > badPair_;   //价格 高
    list<pair<string,string> > pairList_;
    vector<string> goodWordUseful_;
    vector<string> badWordUseful_;
    vector<string> wordSelfLearn_;
    double accu_;
    int cutoff_;
    fstream log_;
    Knowledge* knowledge_;
    Analyzer* analyzer_;
    CMA_Factory* factory_;
    string modelPath_;
    string indexPath_;
    string dictPath_;
public:
    OpinionsClassificationManager(const string& cma_path,const string& path);
    ~OpinionsClassificationManager();
    void GetWordFromTrainData();
    void ClassifyWord(const string& word);
    void ClassifyWordVector();
    bool IsReverse(const string& word);
    void ReverseDeal(vector<string>& wordvec,bool& good);
    void InsertWord(const string& word,bool good);
    void SegQuery(const std::string& query, vector<string>& ret);
    void SegWord(const std::string& Word, vector<string>& ret );
    void GetPairFromTrainData();
    void InsertPair(const string& word1,const string& word2,bool good);
    double PMI(const string& word1,const string& word2);
    void ClassifyPair(const pair<string,string>& p);
    void ClassifyPairVector(const vector<pair<string,string> >& p);
    int GetResult(const string& Sentence);
    bool Include(const pair<string,string>& wordpair,const vector<pair<string,string> >& pairvec);
    bool Include(const string& word, const vector<string>& strvec);
    void Classify(const string& Segment, std::pair<UString,UString>&result);
    void Load(const string& pathname, vector<string>& vec);
    void Load(const string& pathname, vector<pair<string,string> >& vec);
    void LoadAll(const string& path);
    void Save(const string& pathname, vector<string>& vec);
    void Save(const string& pathname, vector<pair<string,string> >& vec);
    void SaveSelect(const string& pathname, vector<string>& vec);
    void SaveAll(const string& path);
    void Sort();
    void Insert(const string& word,int good);
    int DealwithWordPair(const string& wordpair,bool& reverse);
public:
    static std::string system_resource_path_;
};

};
#endif

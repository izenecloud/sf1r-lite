#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_CLASSFICATION_OPINIONMANAGER_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_CLASSFICATION_OPINIONMANAGER_H

#include <icma/icma.h>

#include <string.h>
//#include <icma/icma.h>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mining-manager/auto-fill-submanager/word_leveldb_table.hpp>
#include <icma/icma.h>
#include <string.h>
#include <iostream>


using namespace cma;
//using namespace crfsgd;
using namespace std;

using std::string;
using std::vector;


namespace sf1r
{

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
    void  getWordFromTrainData();
    void  ClassfyWord(string word);
    void  ClassfyWordVector();
    bool  isreverse(string word);
    void  reverseDeal(vector<string>& wordvec,bool& good);
    void  insertWord(string word,bool good);
    vector<string> SegQuery(const std::string& query);
    vector<string> SegWord(const std::string& Word);
    void  getPairFromTrainData();
    void  inserPair(string word1,string word2,bool good);
    double PMI(string word1,string word2);
    void  ClassfyPair(pair<string,string> p);
    void  ClassfyPairVector(vector<pair<string,string> > p);
    int getResult(const string& Sentence);
    bool  include(pair<string,string> wordpair,vector<pair<string,string> >& pairvec);
    bool  include(string word,vector<string>& strvec);
    std::pair<UString,UString> test(const string& Segment);
    void  load(vector<string>& vec,string pathname);
    void  load(vector<pair<string,string> >& vec,string pathname);
    void  loadAll(string path);
    void  save(vector<string>& vec,string pathname);
    void  save(vector<pair<string,string> >& vec,string pathname);
    void  saveSelect(vector<string>& vec,string pathname);
    void  saveAll(string path);
    void  sort();
    void  insert(string word,int good);
    int  dealwithWordPair(string wordpair,bool& reverse);
public:
    static std::string system_resource_path_;
};

};
#endif

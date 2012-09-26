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

class OpinionsClassficationManager
{   
    typedef izenelib::am::leveldb::Table<std::string, int> LevelDBType;
    LevelDBType*                dbTable;
    vector<pair<string,bool> >  trainData;
    vector<string>              badData;
    vector<string>              goodData;
    list<string>                wordList;
    vector<string>              goodWord;    //好
    vector<string>              badWord;     //差
    vector<string>              reverseWord; //不
    vector<string>              reverseData;
    vector<string>              posData;
    vector<string>              pairWord;    //性价比
    vector<pair<string,string> >  goodPair;  //性价比 高
    vector<pair<string,string> >  badPair;   //价格 高
    list<pair<string,string> >    pairList;
    vector<string>              goodWordUseful;
    vector<string>              badWordUseful;
    vector<string>              goodWordSelfLearn;
    vector<string>              badWordSelfLearn;
    vector<string>              WordSelfLearn;
    double                         accu;
    int                            cutoff;
    fstream                        log;
    Knowledge*                   knowledge;
    Analyzer*                    analyzer;
    CMA_Factory*                 factory;
    string                       modelPath;
    string                       IndexPath;
    string                       dictPath;
public:
    OpinionsClassficationManager(string cma_path,string path);
    ~OpinionsClassficationManager();
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
    int  getResult(string Sentence);
    bool  include(pair<string,string> wordpair,vector<pair<string,string> >& pairvec);
    bool  include(string word,vector<string>& strvec);
    std::pair<UString,UString>  test(string Segment);
    void  load(vector<string>& vec,string pathname);
    void  load(vector<pair<string,string> >& vec,string pathname);
    void  loadAll(string path);
    void  save(vector<string>& vec,string pathname);
    void  save(vector<pair<string,string> >& vec,string pathname);
    void  saveSeclect(vector<string>& vec,string pathname);
    void  saveAll(string path);
    void  sort();
    void  insert(string word,int good);
    int  dealwithWordPair(string wordpair,bool& reverse);
public:
    static std::string system_resource_path_;
};

};
#endif

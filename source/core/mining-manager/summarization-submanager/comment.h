
#include <icma/icma.h>

#include "Ngram.h"
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <string.h>

using namespace std;
using namespace cma;

namespace sf1r
{
//class  Ngram;

class OpinionsManager
{
    double SigmaRep;
    double SigmaRead;
    double SigmaSim;
    double SigmaLength;
    std::vector<std::string> Z;
    std::vector<std::pair<std::string,int> >   Micropinion;
    Analyzer* analyzer;
    Knowledge* knowledge;
    CMA_Factory *factory;
    Ngram* Ngram_;
    fstream out;
    int windowsize;
public:
    OpinionsManager(string colPath);//string enc,string cate,string posDelimiter,
    ~OpinionsManager();


public:
    void beginSeg(string inSentence, string& outSentence);
    void stringToWordVector(const std::string& Mi,std::vector<std::string>& words);
    void WordVectorToString(std::string& Mi,const std::vector<std::string>& words);
    void setWindowsize(int C);
    void setComment(std::vector<std::string> Z_);
    void setSigma(double SigmaRep_,double SigmaRead_,double SigmaSim_,double SigmaLength_);
    //score
    double ScoreRep(const std::vector<std::string>& M);
    double ScoreRep(const std::string& Mi);
    double ScoreRead(const std::vector<std::string>& M);
    double ScoreRead(const std::string& Mi);
    double Sim(const std::string& Mi,const std::string& Mj);
    double Sim(std::vector<std::string> wordsi,std::vector<std::string> wordsj);
    double getMScore(const std::vector<std::string>& M);
    double Srep(std::vector<std::string> words);
    double Sread(const std::vector<std::string> words);
    double Score(const std::vector<std::string> words);
    //subject
    bool SubjectLength(const std::vector<std::string>& M);
    bool SubjectRead(const std::string& Mi);
    bool SubjectRead(const std::vector<std::string>& M);
    bool SubjectRep(const std::vector<std::string>& M);
    bool SubjectRep(const std::string& Mi);
    bool SubjectSim(const std::vector<std::string>& M);
    bool Subject(const std::vector<std::string>& M);

    //rep
    double PMIlocal(const std::vector<std::string>& words,const int& offset,int C=3);
    double PMImodified(const std::string& Wi,const std::string& Wj,int C=3);
    double Possib(const std::string& Wi,const std::string& Wj);
    double Possib(const std::string& Wi);
    double CoOccurring(const std::string&Wi,const std::string& Wj,int C=3);
    double CoWord(const std::string& Wi,const std::string& Wj,int C);
    //algorithm
    std::vector<std::string> CandidateWord();
    std::vector<std::pair<std::string,std::string> > seed(std::vector<std::string> CandidateWord);
    std::vector<std::string> seak(const std::vector<std::pair<std::string,std::string> > BigramList);
    bool SimInclude(std::vector<std::string> phrase,double score,std::vector<std::pair<std::vector<std::string>,double> >& candList);
    bool NotMirror(std::vector<std::string> phrase, std::pair<std::string,std::string> bigram);
    std::vector<std::string> Merge(std::vector<std::string> phrase, std::pair<std::string,std::string> bigram);
    std::vector<std::pair<std::string,std::string> > GetJoinList(std::vector<std::string> phrase,std::vector<std::pair<std::string,std::string> > BigramList);
    void GenerateCandidates(std::vector<std::string> newPhrase,double score,std::vector<std::pair<std::vector<std::string>,double> >& candList,std::vector<std::vector<std::string> >& seedGrams);
    std::vector<std::vector<std::string> > getMicroOpinions(std::vector<std::pair<std::vector<std::string>,double> >& candList);
    std::vector<std::string> get();
    std::vector<std::pair<std::string,double> > changeForm(std::vector<std::pair<std::vector<std::string>,double> >& candList);
   
    //std::vector<std::string> PingZhuang( std::vector<std::vector<std::string> > candString);
    std::vector<std::string> PingZhuang( std::vector<std::pair<std::string,double> > candString);
    bool add(std::vector<std::string>& candVector,std::vector<std::string> candString);
    std::string getSentence(std::vector<std::string>& candVector);
  /* */
};

};

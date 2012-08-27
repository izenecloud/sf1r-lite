#include "comment.h"
#include <math.h>   
using namespace std;



typedef std::pair<std::string,int> seedPair;

struct seedPairCmp {
  bool operator() (const seedPair seedPair1,const seedPair seedPair2) 
        { 
             return seedPair1.second > seedPair2.second;
        }

} seedPairCmp;
string k="中";
const int cnCharSize=k.size();
namespace sf1r
{
OpinionsManager::OpinionsManager(string colPath)//string enc,string cate,string posDelimiter,
{/*
    factory = CMA_Factory::instance();
    knowledge = factory->createKnowledge();
    knowledge->loadModel( enc.data(), cate.data() );
    analyzer = factory->createAnalyzer();
    analyzer->setOption(Analyzer::OPTION_TYPE_POS_TAGGING,0);
    analyzer->setOption(Analyzer::OPTION_ANALYSIS_TYPE,77);
    analyzer->setKnowledge(knowledge);
    analyzer->setPOSDelimiter(posDelimiter.data());
*/
    Ngram_= new Ngram(colPath);
    string logpath=colPath+"/OpinionsManager.log";
    out.open(logpath.c_str(), ios::out);
    windowsize=3;

}
OpinionsManager::~OpinionsManager()
{   out.close();
    //delete factory;
    delete knowledge;
    delete analyzer;

}
void OpinionsManager::setWindowsize(int C)
{
    windowsize=C;
}
void OpinionsManager::setComment(std::vector<std::string> Z_)
{
         Z=Z_;
         for(int i=0;i<Z.size();i++)
        { out<<Z[i]<<endl;
        }
}
void OpinionsManager::setSigma(double SigmaRep_,double SigmaRead_,double SigmaSim_,double SigmaLength_)
{
   SigmaRep=SigmaRep_;
   
   SigmaRead=SigmaRead_;
   SigmaSim=SigmaSim_;
   SigmaLength=SigmaLength_;
   out<<"SigmaRep:"<<SigmaRep<<endl;
   out<<"SigmaRead:"<<SigmaRead<<endl;
   out<<"SigmaSim:"<<SigmaSim<<endl;
   out<<"SigmaLength:"<<SigmaLength<<endl;
}
void OpinionsManager::beginSeg(string inSentence, string& outSentence)
{
    char* charOut;
    analyzer->runWithStream(inSentence.c_str(),charOut);
    string outString(charOut);
    outSentence=outString;

}
void OpinionsManager::stringToWordVector(const std::string& Mi,std::vector<std::string>& words)
{
    izenelib::util::UString ustr(Mi, izenelib::util::UString::UTF_8);

    string strseg;
    for(int i=0;i<ustr.length();i++)
    {  
       
       ustr.substr(i,1).convertString(strseg, UString::UTF_8);
       words.push_back(strseg);
    }

    //分词//TODO
};
void OpinionsManager::WordVectorToString(std::string& Mi,const std::vector<std::string>& words)
{
   Mi="";
   for(int i=0;i<words.size();i++)
   {
       Mi=Mi+words[i];
   }

    //TODO
};
//score
double OpinionsManager::ScoreRep(const std::vector<std::string>& M)
{
    double Srep=0;
    for(int i=0; i<M.size(); i++)
    {
        Srep+=ScoreRep(M[i]);
    }
    return Srep;
};
double OpinionsManager::ScoreRep(const std::string& Mi)
{
    std::vector<std::string> words;
    stringToWordVector(Mi,words);
    return Srep(words);
};
double OpinionsManager::Srep(std::vector<std::string> words)
{
    
   
    int n=words.size();

    //if(windowsize==6)
   // {out<<"size:"<<n<<endl;}
    double sigmaPMI=0;
    for(int i=0; i<n; i++)
    {
        //out<<"words"<<words[i]<<endl;
        sigmaPMI+=PMIlocal(words,i,windowsize);
        // if(windowsize==6)
        // {out<<"PMI:"<<PMIlocal(words,i,windowsize)<<endl;}
    }
    return sigmaPMI/double(n);
};
double OpinionsManager::ScoreRead(const std::vector<std::string>& M)
{
    double Sread=0;
    for(int i=0; i<M.size(); i++)
    {
        Sread+=ScoreRead(M[i]);
    }
    return Sread;
};
double OpinionsManager::ScoreRead(const std::string& Mi)
{
    std::vector<std::string> words;
    stringToWordVector(Mi,words);
    Sread(words);

};
double OpinionsManager::Sread(const std::vector<std::string> words)
{
    double possiblity=1;
    int K=0;
    for(int i=0; i<words.size(); i++)
    {
        //out<<"words"<<words[i]<<endl;
        if(i==0){ }
            else
            { 
            if(i==1)
            {
                possiblity*=Ngram_->Possiblity(words[1],words[0]);
                //out<<words[1]<<"|"<<words[0]<<"Possiblity"<<Ngram_->Possiblity(words[1],words[0])<<endl;
                //out<<"possiblity"<<possiblity<<endl;
                K+=1;
            }
            else
            {
                if(i>=2)
                {
                    possiblity*=Ngram_->Possiblity(words[i],words[i-1],words[i-2]);
                    K+=1;
                }
                /*
                else
                {
                    if(i>=3)
                    {
                        possiblity*=Ngram_->Possiblity(words[i],words[i-1],words[i-2],words[i-3]);    //TODO
                        K+=3;
                    }
                }
                */
            }
            }

    }
    //out<<"possiblity"<<possiblity<<endl;
    double score=log(possiblity)/log(double(2))/K;
    //out<<"score"<<score<<endl;
    return score;
};
double OpinionsManager::Score(const std::vector<std::string> words)
{
    return Sread(words)+Srep(words)+0.25*words.size()-0.5;
}
double OpinionsManager::Sim(const std::string& Mi,const std::string& Mj)
{
    std::vector<std::string> wordsi,wordsj;
    stringToWordVector(Mi,wordsi);
    stringToWordVector(Mi,wordsj);
    return Sim(wordsi,wordsj);

};
double OpinionsManager::Sim(std::vector<std::string> wordsi,std::vector<std::string> wordsj)
{
    for (int j=0; j<wordsi.size(); j++)
    {
        // out<<wordsi[j];
    }
    for (int j=0; j<wordsj.size(); j++)
    {
        // out<<wordsj[j];
    }
    //out<<endl;
    int sizei=wordsi.size(),sizej=wordsj.size();
    std::vector<std::string> wordsij=wordsi;
    wordsij.insert(wordsij.begin(),wordsj.begin(),wordsj.end());
    sort(wordsij.begin(),wordsij.end());  //
    wordsij.erase(unique(wordsij.begin(), wordsij.end()),wordsij.end()); //
    int sizeij=wordsij.size();
    //out<<"sizei"<<sizei<<"sizej"<<sizei<<"sizeij"<<sizeij;
    return double((sizei+sizej-sizeij))/double(sizeij);//Jaccard similarity


};
double OpinionsManager::getMScore(const std::vector<std::string>& M)
{
    return ScoreRep(M)+ScoreRead(M);
};

//subject
bool OpinionsManager::SubjectLength(const std::vector<std::string>& M)
{
    int Length=0;
    for(int i=0; i<M.size(); i++)
    {
        Length+=M[i].length();
    }
    return Length<SigmaLength;
}

bool OpinionsManager::SubjectRead(const std::string& Mi)
{
    return ScoreRead(Mi)>SigmaRead;
}
bool OpinionsManager::SubjectRead(const std::vector<std::string>& M)
{
    for(int i=0; i<M.size(); i++)
    {
        if(!SubjectRead(M[i]))
        {
            return false;
        }
    }
    return true;
}
bool OpinionsManager::SubjectRep(const std::string& Mi)
{
    return ScoreRep(Mi)>SigmaRep;
}
bool OpinionsManager::SubjectRep(const std::vector<std::string>& M)
{
    for(int i=0; i<M.size(); i++)
    {
        if(!SubjectRep(M[i]))
        {
            return false;
        }
    }
    return true;
}
bool OpinionsManager::SubjectSim(const std::vector<std::string>& M)
{
    for(int i=0; i<M.size()-1; i++)
    {
        for(int j=i+1; j<M.size(); j++)
        {
            if(Sim(M[i],M[j])<SigmaSim)
            {
                return false;
            }
        }
    }
    return true;
}
bool OpinionsManager::Subject(const std::vector<std::string>& M)
{
    return SubjectLength(M)&&SubjectRead(M)&&SubjectRep(M)&&SubjectSim(M);
}

//rep
double OpinionsManager::PMIlocal(const std::vector<std::string>& words,const int& offset,int C)//C=window size
{
    int k=0;
    double Spmi=0;
    int size=words.size();
    for(int i=max(0,offset-C); i<min(size,offset+C); i++)//
    {
        //out<<"pmi"<<endl;
        if(i!=offset)
        {
            k++;
            Spmi+=PMImodified(words[offset],words[i],C);
            //if(windowsize==6){out<<"PMImodified:"<<PMImodified(words[offset],words[i],C)<<endl;}
        }

    }
    //if(windowsize==6){out<<"k"<<k<<endl;}
    return Spmi/double(k);
}

double OpinionsManager::PMImodified(const std::string& Wi,const std::string& Wj,int C)
{
    if(windowsize==6)
    {//out<<Wi<<Wj<<"Possib(Wi,Wj)"<<Possib(Wi,Wj) <<"CoOccurring(Wi,Wj,C)"<<CoOccurring(Wi,Wj,C)<<"Possib(Wi)"<<Possib(Wi)<<"Possib(Wj)"<<Possib(Wj)<<"windowsize"<<C<<endl;
    }
     int s=Z.size();
     return log((Possib(Wi,Wj)*CoOccurring(Wi,Wj,C))*max(double(s),10+Z.size()*0.5)/(Possib(Wi)*Possib(Wj)))/log(2);
}

double OpinionsManager::CoOccurring(const std::string& Wi,const std::string& Wj,int C)
{
   // if(windowsize==6){return CoWord(Wi,Wj,C);}
    int Poss=0;
    for(int j=0; j<Z.size(); j++)
    {
        int posi=-1;
        int posj=-1;
        std::vector<std::string> wordsi;
        stringToWordVector(Z[j],wordsi);
        for(int i=0; i<wordsi.size(); i++)
        {
            if(wordsi[i]==Wi)
            {
                posi=i;
            }
            else
            {
                if(wordsi[i]==Wj)
                {
                    posj=i;
                }
            }
            if(abs(posj-posi)<C&&posi!=-1&&posj!=-1)//||abs(posi-posi)<C+wordsi[i].length()-cnCharSize//del abs
            {
                Poss++;
                break;
            }
        }

    }
   
    return Poss;

}
double OpinionsManager::CoWord(const std::string& Wi,const std::string& Wj,int C)
{
    int Poss=0;
    for(int j=0; j<Z.size(); j++)
    {
        int posi=-2;
        int posj=-2;
        int posStart=0;
        bool ismaller=true;
        while(posi==string::npos||posj==string::npos)
        {
           if(ismaller)
           {
              posi=Z[j].find(Wi,posStart);
           }
           else
           {
              posj=Z[j].find(Wj,posStart);
           }
                  if(posi==string::npos||posj==string::npos)//||abs(posi-posi)<C+wordsi[i].length()-cnCharSize
                  {
                    
                      break;
                   }
                  if((abs(posj-posi)<C)&&posi!=-2&&posj!=-2)//||abs(posi-posi)<C+wordsi[i].length()-cnCharSize
                  {
                     Poss++;
                      break;
                   }
           if(posi<posj)
           {ismaller=true;}
           else
           {ismaller=false;}
           posStart=min(posi,posj)+1;
        }
    }
    return Poss;
}
double OpinionsManager::Possib(const std::string& Wi,const std::string& Wj)
{
    int Poss=0;
    for(int j=0; j<Z.size(); j++)
    {
        int loci = Z[j].find(Wi);
        int locj = Z[j].find(Wj);
        if( locj!= string::npos&&loci!= string::npos  )
        {
            Poss++;
        }

    }
    return Poss;

}
double OpinionsManager::Possib(const std::string& Wi)
{
    int Poss=0;
   // out<<"string"<<Wi<<endl;
    for(int j=0; j<Z.size(); j++)
    {
        
        int loc = Z[j].find(Wi);
        //out<<"j"<<j<<endl;//loc"<boost::lexical_cast<string>(loc)<<
        if( loc!= string::npos )
        {
            Poss++;
        }

    }
    return Poss;
}
std::vector<std::string> OpinionsManager::CandidateWord()
{
    //out<<"Candidate"<<endl;
    std::vector<std::string> wordlist;
    for(int j=0; j<Z.size(); j++)
    {
        std::vector<std::string> words;
        stringToWordVector(Z[j],words);
        for(std::vector<std::string>::iterator it=words.begin(); it!=words.end(); it++)
        {
             // out<<"words"<<(*it)<<endl;
        }
        wordlist.insert(wordlist.end(),words.begin(),words.end());

    }
    sort(wordlist.begin(),wordlist.end());
    //out<<"wordlist"<<wordlist.size()<<endl;
    for(std::vector<std::string>::iterator it=wordlist.begin(); it!=wordlist.end(); it++)
    {
       //out<<"wordList"<<(*it)<<endl;
    }
   // std::vector<std::string>::iterator iter = unique(wordlist.begin(), wordlist.end());
    std::vector<std::pair<std::string,int> > resultList;
    string wordTemp="";
    //out<<"cand"<<endl;
    for(std::vector<std::string>::iterator it=wordlist.begin(); it!=wordlist.end(); it++)
    {
        if (wordTemp!=(*it))
        {
            //out<<"wordTemp"<<wordTemp<<endl;
            int times=Possib((*it));
            if(times>1)
            {
                resultList.push_back(std::make_pair((*it), times));
            }
        }
        wordTemp=(*it);
    }
    sort(resultList.begin(),resultList.end(),seedPairCmp);
    std::vector<std::string> CandidateWord;
    int j=resultList.size();
    //out<<"j"<<j<<endl;
    for(int i=0; i<min(50,j); i++)
    {
        if(resultList[i].first!="，"&&resultList[i].first!="。"&&resultList[i].first!="!"&&resultList[i].first!="！")
        {
        CandidateWord.push_back(resultList[i].first);
       // out<<"seed word"<<resultList[i].first<<"freq:"<<resultList[i].second<<endl;
        }
    }
    
    return CandidateWord;

}
std::vector<std::pair<std::string,std::string> > OpinionsManager::seed(std::vector<std::string> CandidateWord)
{
    std::vector<std::pair<std::string,std::string> > n;
    if (CandidateWord.empty())
    {return n;}
    std::vector<std::pair<std::string,std::string> > resultList;
    std::vector<std::string> Bigram;
    //out<<"CandidateWord.size()"<<CandidateWord.size()<<endl;
    for(int i=0; i<CandidateWord.size(); i++)
    {
        for(int j=0; j<CandidateWord.size(); j++)
        {
            Bigram.clear();
            Bigram.push_back(CandidateWord[i]);
            Bigram.push_back(CandidateWord[j]);
            //out<<CandidateWord[i]<<CandidateWord[j]<<"Srep(Bigram)"<<Srep(Bigram)<<"Sread(Bigram)"<<Sread(Bigram)<<endl;
            //out<<"here1"<<endl;
            if(Sread(Bigram)>=SigmaRead &&Srep(Bigram)>=SigmaRep)
            {
                 //out<<"here2"<<endl;
                 resultList.push_back(std::make_pair(CandidateWord[i],CandidateWord[j]));
                 //out<<"seed pair"<<CandidateWord[i]<<CandidateWord[j]<<endl;
            }
            //out<<"i"<<i<<"j"<<j<<endl;
        }
    }
   // out<<"finish"<<endl;
    for(int i=0; i<resultList.size(); i++)
    {
        //out<<"seed pair"<<resultList[i].first<<resultList[i].second<<endl;
    }
    /**/
    return resultList;
}
std::vector<std::string> OpinionsManager::seak(const std::vector<std::pair<std::string,std::string> > BigramList)
{
    std::vector<std::string> n;
    if (BigramList.empty())
    {return n;}
    //out<<"here1"<<endl;
    std::vector<std::pair<std::string,std::string> > joinList=BigramList;
    //out<<"here2"<<endl;
    std::vector<std::vector<std::string> > seedGrams;
    //out<<"here3"<<endl;
    std::vector<std::pair<std::vector<std::string>,double> > candList;
    //out<<"BigramList"<<BigramList.size()<<endl;
    bool del;
    for(int i=0; i<BigramList.size(); i++)
    {
        std::vector<std::string> phrase;
        phrase.push_back(BigramList[i].first);
        phrase.push_back(BigramList[i].second);
        seedGrams.push_back(phrase);
    }
    //out<<"push_back"<<endl;
    for(int i=0; i<seedGrams.size(); i++)
    {
        std::vector<std::string> phrase=seedGrams[i];
       
        //out<<"seedGrams[i]"<<endl;
        for (int j=0; j<phrase.size(); j++)
        {
                    //  out<<phrase[j];
        }
        //out<<endl; 
        if (Sread(phrase)<SigmaRead&&Srep(phrase) < SigmaRep)
        {   
            for (int j=0; j<phrase.size(); j++)
            {
               //out<<phrase[j]<<endl;
            }
            //out<<Sread(phrase)<<"  "<<Srep(phrase)<<endl;
            continue;
        }
        if(SimInclude(phrase,Score(phrase),candList))
        {
           // out<<"simlar"<<endl;
            continue;
        } 
        
       
        std::vector<std::pair<std::string,std::string> > joinList=GetJoinList(phrase,BigramList);
        //out<<"joinList"<<joinList.size()<<endl;
        del=false;
        // out<<"del"<<del<<endl;
        for (int j=0; j<joinList.size(); j++)
        {
             
            std::pair<std::string,std::string>  bigram=joinList[j];
            //out<<" bigram.first"<< bigram.first<<" bigram.second"<< bigram.second<<endl;
            if( NotMirror(phrase, bigram))
            {
                //out<<"not mirror"<<endl;
                std::vector<std::string>  newPhrase=Merge(phrase, bigram);
                //out<<"newPhrase";
                for (int j=0; j<newPhrase.size(); j++)
                {
                    //  out<<newPhrase[j];
                }
                //out<<endl;
                //out<<Sread(newPhrase)<<"  "<<Srep(newPhrase)<<endl;
                //out<<"seedGrams.size()"<<seedGrams.size()<<endl;
                if (Sread(newPhrase)>SigmaRead&&Srep(newPhrase)>SigmaRep)
                { 
                   // //  // 
                       
                        std::vector<std::string> bigramVector;
                        bigramVector.push_back(bigram.first);
                        bigramVector.push_back(bigram.second);
                       // if (Score(newPhrase)>Score(phrase)||Score(newPhrase)>Score(bigramVector))
                       // { 
                       // seedGrams.erase(find(seedGrams.begin(),seedGrams.end(),bigramVector));
                       // seedGrams.erase(find(seedGrams.begin(),seedGrams.end(),phrase));
                         std::vector<std::pair<std::vector<std::string>,double> >::iterator pos=find(candList.begin(),candList.end(),std::make_pair(bigramVector,Score(bigramVector)))  ;  
                          if(pos!= candList.end())
                          {
                           candList.erase(pos);
                          }
                          pos=find(candList.begin(),candList.end(),std::make_pair(phrase,Score(phrase)));
                          if(pos!= candList.end())
                          {
                           candList.erase(pos);
                          }
                       // out<<"candList.size()"<<candList.size()<<endl;
                        GenerateCandidates(newPhrase,Score(newPhrase),candList,seedGrams);
                        bool del=true;
                       // }
                }
                
                //out<<"after add seedGrams.size()"<<seedGrams.size()<<endl;

            }
        }
        //out<<"del"<<del<<endl;
        if(!del)//&&phrase.size()==2
        {
            //out<<"add"<<endl;
            candList.push_back(std::make_pair(phrase,Score(phrase)));
            //out<<"candList.size()"<<candList.size()<<endl;
        }
    }
    //out<<"finish"<<endl;
   // out<<"candList.size()"<<candList.size()<<endl;
    std::vector<std::pair<std::string,double> > candString=changeForm(candList);
     //std::vector<std::vector<std::string> > cand;
    std::vector<std::string>  cand;
    for(int i=0;i<candString.size();i++)
    {
         //out<<"Opinion"<<candString[i].first<<"score"<<candString[i].second<<endl;
        // cand.push_back(candList[i].first);
         cand.push_back(candString[i].first);
    }
    //PingZhuang(cand);
    return PingZhuang(candString);

}

std::vector<std::string> OpinionsManager::PingZhuang(std::vector<std::pair<std::string,double> >  candString)
{
  std::vector<std::string> tingyongList;
   tingyongList.push_back("那就是");
   tingyongList.push_back("看上去");
   tingyongList.push_back("看起来");
   tingyongList.push_back("因为");
   tingyongList.push_back("虽然");
   tingyongList.push_back("但是");
   tingyongList.push_back("而且");
   tingyongList.push_back("虽然");
   tingyongList.push_back("就是");
   tingyongList.push_back("不但");
   tingyongList.push_back("还是");
   tingyongList.push_back("结果");
   tingyongList.push_back("但");
   tingyongList.push_back("就");
   tingyongList.push_back("如果");
   tingyongList.push_back("另外");
   
     
     
  
  //out<<"pingzhuang"<<endl;
  std::vector<std::string> result;
  for(int i=0;i<Z.size();i++)
  {
    string thisString=Z[i];
    
    boost::algorithm::replace_all(thisString, " ", ",");
  
    boost::algorithm::replace_all(thisString, "。", ",");
    boost::algorithm::replace_all(thisString, "，", ",");
    boost::algorithm::replace_all(thisString, "！", ",");
    string restString=thisString;
    string todoString;
    int pos=restString.find(",");
   //out<<"i"<<i<<endl;
    while(restString.find(",")!=string::npos)
    {
       todoString=restString.substr(0,pos);
       restString=restString.substr(pos+1);
       int count=0;
       double score=0;
       std::vector<std::pair<std::string,double> > numList;
       for(int j=0;j<candString.size();j++)
       {  
         // out<<"j"<<j<<endl;
          if(todoString.find(candString[j].first)!=string::npos)
          {
                score+=candString[j].second;
            
                numList.push_back(candString[j]);
               // break;
          }
           
       }
       if(score>0.3)
       {
           for(int j=0;j<tingyongList.size();j++)
           { 
              if(todoString.substr(0,tingyongList[j].length())==tingyongList[j])
              {  
                 todoString=todoString.substr(tingyongList[j].length());
              }
           }
           result.push_back(todoString);//去除 虽然  但是 而且 就是 不但 但 还是
           for(int j=0;j<numList.size();j++)
           { 
               
               candString.erase(find(candString.begin(),candString.end(),numList[j]));
           }
       }
       pos=restString.find(",");
    }
  }
  return result;

}

/*
std::vector<std::string> OpinionsManager::PingZhuang( std::vector<std::vector<std::string> > candString)
{
    std::vector<std::string> n;
    if(candString.empty())
    {return n;}
    out<<"PingZhuang"<<endl;
    setWindowsize(3);
    std::vector<std::string> candVector;
    std::vector<std::string> sentence;
    //candVector.push_back(candString[0]);
    candVector=candString[0];
    candString.erase(find(candString.begin(),candString.end(),candString[0]));
    out<<"ready"<<endl;
    while(!candString.empty())
    {   //out<<"candVector:";
        for (int j=0; j<candVector.size(); j++)
        {
                   // out<<candVector[j];
        }
        //out<<endl;
      
      for(int i=0;i<candString.size();i++)
      {
            //out<<"add";
            for (int j=0; j<candString[i].size(); j++)
            {
                    //  out<<candString[i][j];
            }
            //out<<endl;
            if(add(candVector,candString[i]))
            {  
            candString.erase(find(candString.begin(),candString.end(),candString[i]));
            //out<<"success"<<endl;
            for (int j=0; j<candVector.size(); j++)
            {
                     // out<<candVector[j];
            }
            //out<<endl;
            break;
            }
          else
          {
             if(i==candString.size()-1)
              {
                sentence.push_back(getSentence(candVector));
                
                candVector.clear();
                //candVector.push_back(candString[0]);
                candVector=candString[0];
                candString.erase(find(candString.begin(),candString.end(),candString[0]));
                break;
              }
          }
      }
      
    } 
    setWindowsize(3);  
    return sentence; 
   
   
}
*/
bool OpinionsManager::add(std::vector<std::string>& candVector,std::vector<std::string> candString)
{
   int score1=0,score2=0;
  //out<<"add"<<endl;
  
  std::vector<std::string> testVector1=candVector;
  //testVector.push_back(candString);
    testVector1.insert(testVector1.end(),candString.begin(),candString.end());
  //out<<"Srep"<<Srep(testVector1)<<endl;
  //out<<"Sread"<<Sread(testVector1)<<endl;
 if((Srep(testVector1)>SigmaRep-0.25*testVector1.size()&&Sread(testVector1)>SigmaRead-0.25*testVector1.size())&&Srep(testVector1)>SigmaRep)
  {
       score1=Score(testVector1);
      // return true;
  }
  
  
  
  std::vector<std::string> testVector2=candVector;
    //testVector.push_back(candString);
    testVector2.insert(testVector2.begin(),candString.begin(),candString.end());
  //out<<"Srep"<<Srep(testVector2)<<endl;
  //out<<"Sread"<<Sread(testVector2)<<endl;
  if((Srep(testVector2)>SigmaRep-0.25*testVector2.size()&&Sread(testVector2)>SigmaRead-0.25*testVector2.size())&&Srep(testVector2)>SigmaRep)
  {
       score2=Score(testVector2);
      // return true;
  }
  if(score1==0&&score2==0)
  {return false;}
  else
  {
  if(score1>score2)
  {
  candVector.insert(candVector.end(),candString.begin(),candString.end());
  }
  else
  {
  candVector.insert(candVector.begin(),candString.begin(),candString.end());
  }
  return true;
  }
  
}


std::string OpinionsManager::getSentence(std::vector<std::string>& candVector)
{
       // out<<"Sentence";
        string temp="";
        for (int j=0; j<candVector.size(); j++)
        {
                      out<<candVector[j];
                      temp+=candVector[j];
        }
        out<<endl;
        return temp;
}
/*
*/
bool OpinionsManager::SimInclude(std::vector<std::string> phrase,double score,std::vector<std::pair<std::vector<std::string>,double> >& candList)
{
    if(candList.empty()){return false;}
    else
    {
    for(std::vector<std::pair<std::vector<std::string>,double> >::iterator it=candList.begin(); it!=candList.end(); it++)
    {
        //out<<"sim"<<Sim(phrase,(*it).first)<<endl;
        if(Sim(phrase,(*it).first)>SigmaSim)
        {
            if(score >(*it).second)
            {
                if(phrase==(*it).first){return false;}
                else
                {
                for (int j=0; j<phrase.size(); j++)
                {
                     // out<<phrase[j];
                }
                //out<<endl;
                for (int j=0; j<(*it).first.size(); j++)
                {
                     // out<<(*it).first[j];
                }
                //out<<endl;
                candList.erase(it);
                if(phrase.size()==2)
                {
                    candList.push_back(std::make_pair(phrase,score));
                }
                }
                //return false;
            }
            else
            {
               return true;
            }
        }
    }
    return false;
    }
}
bool OpinionsManager::NotMirror(std::vector<std::string> phrase, std::pair<std::string,std::string> bigram)
{
    if((phrase[phrase.size()-2]==bigram.first&&phrase[phrase.size()-1]==bigram.second)||(phrase[phrase.size()-1]==bigram.first&&phrase[phrase.size()-2]==bigram.second))
    {
        return false;
    }
    else
    {
        return true;
    }
}
std::vector<std::string> OpinionsManager::Merge(std::vector<std::string> phrase, std::pair<std::string,std::string> bigram)
{
    phrase.push_back(bigram.second);
    return phrase;
}
std::vector<std::pair<std::string,std::string> > OpinionsManager::GetJoinList(std::vector<std::string> phrase,std::vector<std::pair<std::string,std::string> > BigramList)
{
    std::vector<std::pair<std::string,std::string> > resultList;
    for(int i=0; i<BigramList.size(); i++)
    {
        if(phrase[phrase.size()-1]==BigramList[i].first)
        {
            resultList.push_back(BigramList[i]);
        }
    }
    return resultList;

}
void OpinionsManager::GenerateCandidates(std::vector<std::string> newPhrase,double score,std::vector<std::pair<std::vector<std::string>,double> >& candList,std::vector<std::vector<std::string> >& seedGrams)
{
    if(!SimInclude(newPhrase,score,candList))
    {
        //candList.push_back(std::make_pair(newPhrase,score));
        seedGrams.push_back(newPhrase);
    }
}
std::vector<std::pair<std::string,double> > OpinionsManager::changeForm(std::vector<std::pair<std::vector<std::string>,double> >& candList)
{ 
   std::vector<std::pair<std::string,double> > newForm;
   string temp;
   for(int i=0;i<candList.size();i++)
   {
      
       WordVectorToString(temp,candList[i].first);
      
       newForm.push_back(std::make_pair(temp,candList[i].second));
   }
   return newForm;
}
std::vector<std::vector<std::string> > OpinionsManager::getMicroOpinions(std::vector<std::pair<std::vector<std::string>,double> >& candList)
{
    std::vector<std::vector<std::string> > MicroOpinions;
    sort(candList.begin(),candList.end());
    int length=0;
    int i=0;
    while(length<SigmaLength&&i<candList.size())
    {
        length+=candList[i].first.size();//TODO length()
        i++;
        MicroOpinions.push_back(candList[i].first);
    }
    return MicroOpinions;
}
std::vector<std::string> OpinionsManager::get()
{  
   //out<<"one product"<<endl;
  
   std::vector<std::string> result=seak(seed(CandidateWord()));
  // string temp;
   for(int i=0;i<result.size();i++)
   {
      // WordVectorToString(temp,result[i]);
       if(result[i].length()>10)
       {
       out<<"MicroOpinions:"<< result[i]<<endl;
       }
   }
out<<"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
   return result;
}
};

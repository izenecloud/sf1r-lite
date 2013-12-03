#include "latticelm.h"
#include <boost/filesystem.hpp> 
#include <util/ustring/UString.h>

using namespace std; 
using namespace latticelm;
namespace bfs = boost::filesystem;

class LatticeLMMap
{
private:
    std::map<std::string,LatticeLM*> latticelm_;
    std::string worddir_;
    std::string traindir_;
public:
    LatticeLMMap(const std::string &worddir,const std::string &traindir):worddir_(worddir),traindir_(traindir)
    {
    }
    ~LatticeLMMap()
    {
        for(std::map<std::string,LatticeLM*>::iterator iter=latticelm_.begin(); iter!=latticelm_.end(); iter++)
        {
            delete iter->second;
        }
    }
    bool segWord(const std::string& sentence,const std::string& category,string& ret)
    {
        //cout<<"segWord"<<endl;
        LatticeLM* lm=getLanguageModel(category);
        if(!lm) 
        {
            return false;
        }
        else
        {
            string spcaesentence=sentence;
            bool lengthlimit=AddSpace(spcaesentence);
            if(lengthlimit)
                 ret=lm->segString(spcaesentence);
            else
                 return false;
        }
        return true;
    }
    void load()
    {
        const bfs::directory_iterator kItrEnd;
        if (!boost::filesystem::is_directory(traindir_))
        {
            cout<<"directory "<<traindir_<<" not found"<<endl;
            return;
        }
        for (bfs::directory_iterator itr(traindir_); itr != kItrEnd; ++itr)
        {
            if (bfs::is_regular_file(itr->status()))
            {
                std::string fileName = itr->path().string();
                if(fileName.find(".train")!=string::npos)
                {
                    AddLanguageModel(fileName.substr(fileName.find("/")+1,fileName.length()-6-fileName.find("/")-1));
                }
                
            }
        }
    }
private:
    LatticeLM* getLanguageModel(const string &category)
    {
        std::map<std::string,LatticeLM*>::iterator it = latticelm_.lower_bound(category);
        it--;
        if(it==latticelm_.end())
        {
            //cout<<"get LanguageModel error! No LanguageModel "<<category<<" detected!"<<endl;
            return NULL;
        }
        //cout<<category<<"  ";
        //cout<<it->first<<"  ";

        if(category.find(it->first)==string::npos)
            return NULL;
        //it->second->TimeShow();
        return it->second;
    }
    bool AddSpace(std::string &sentence)
    {

        string ret;
        size_t size=0;
        izenelib::util::UString usentence(sentence,izenelib::util::UString::UTF_8);
        izenelib::util::UString uret;
        for(size_t i=0;i<usentence.size();i++)
        {
            if((i<usentence.size()-1)&&(usentence.isChineseChar(i)|usentence.isChineseChar(i+1)))
            {
                uret.append(usentence.substr(i,1).append(izenelib::util::UString(" ", izenelib::util::UString::UTF_8)));
                size++;
            }
            else 
                uret.append(usentence.substr(i,1));
        }
        uret.convertString(ret, izenelib::util::UString::UTF_8);
        //cout<<"ret"<<ret<<endl;
/**/
        sentence=ret;//TODO
        if(uret.size()>=1)
            return true;
        else
            return false;
    }
    void AddLanguageModel(const std::string &category)
    {    
        LatticeLM* latticeLm=new LatticeLM;
        string trainfile=traindir_+"/"+category+".train";
        string symbolefile=worddir_+"/"+category+".word";
        cout<<"Adding LanguageModel"<<category<<"..."<<trainfile<<"  "<<symbolefile<<endl;
        try
        {
            if (!boost::filesystem::exists(symbolefile)||!boost::filesystem::exists(trainfile))
            {
                 cout<<category<<" path does not exist."<<endl;
                 return; 
            }
        }
        catch (boost::filesystem::filesystem_error& e)
        {
            cout<<category<<" path does not exist."<<endl;
        }
        latticeLm->loadFile(trainfile,symbolefile);
        latticelm_.insert(pair<string,LatticeLM*>(category,latticeLm));        
    }
};





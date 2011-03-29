///
/// @file   POSUtil.hpp
/// @brief  Provide some function by using POS information for mining component.
/// @author Jia Guo
/// @date   2010-01-08
/// @date   2010-01-08
///
#ifndef POSUTIL_HPP_
#define POSUTIL_HPP_
#include <string>

#include <mining-manager/MiningManagerDef.h>
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <am/3rdparty/rde_hash.h>
#include <util/hashFunction.h>
#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp> 
#include <fstream>
#include <cache/IzeneCache.h>
#include <util/izene_serialization.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <util/functional.h>
namespace sf1r
{

    
class POSParam
{
    
    public:
        POSParam(const std::string mainTag, const std::vector<std::string>& tagList):mainTag_(mainTag), tagList_(tagList)
        {

        }
    public:
        std::string mainTag_;
        std::vector<std::string> tagList_;
        izenelib::am::rde_hash<std::string, bool> tagMap_;
};

class POSResult
{
    public:
        POSResult():allCount_(0)
        {
        }
        POSResult(const std::string& tag):mainTag_(tag), allCount_(0)
        {
        }
        void addCount(uint32_t count = 1)
        {
            allCount_ += count;
        }
        void addPrefix(const std::string& term, uint32_t count = 1)
        {
            uint32_t* index = index_.find(term);
            if( index == NULL )
            {
                index_.insert(term, prefix_.size());
                prefix_.push_back(std::make_pair(term, count));
                suffix_.push_back(std::make_pair(term, 0));
            }
            else
            {
                prefix_[*index].second += count;
            }
        }
        
        void addSuffix(const std::string& term, uint32_t count = 1)
        {
            uint32_t* index = index_.find(term);
            if( index == NULL )
            {
                index_.insert(term, prefix_.size());
                suffix_.push_back(std::make_pair(term, count));
                prefix_.push_back(std::make_pair(term, 0));
            }
            else
            {
                suffix_[*index].second += count;
            }
        }
        
        friend class boost::serialization::access;
    
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {

            ar  & mainTag_ & allCount_ & prefix_ & suffix_;
            
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar  & mainTag_ & allCount_ & prefix_ & suffix_;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        void sort()
        {
            typedef izenelib::util::second_greater<std::pair<std::string, uint32_t> > greater_than;
            std::sort(prefix_.begin(), prefix_.end(), greater_than());
            std::sort(suffix_.begin(), suffix_.end(), greater_than());
        }
    public:
        std::string mainTag_;
        uint32_t allCount_;
        std::vector<std::pair<std::string, uint32_t> > prefix_;
        std::vector<std::pair<std::string, uint32_t> > suffix_;
        izenelib::am::rde_hash<std::string, uint32_t> index_;
};



class POSParamList
{
    public:
        POSParamList()
        {
        }
        POSParamList(const std::vector<POSParam>& paramList):paramList_(paramList)
        {
            for(uint32_t i=0;i<paramList_.size();i++)
            {
                for(uint32_t j=0;j<paramList_[i].tagList_.size();j++)
                    tagMap_.insert(paramList_[i].tagList_[j], i);
            }
        }
        
        bool getMainTag(const std::string& input, std::string& mainTag)
        {
            uint32_t* pos = tagMap_.find(input);
            if( pos == NULL ) return false;
            mainTag = paramList_[*pos].mainTag_;
            return true;
        }


        
    public:
        std::vector<POSParam> paramList_;
        izenelib::am::rde_hash<std::string, uint32_t> tagMap_;
        
};
class POSResultList
{
    public:
        POSResultList(const POSParamList& paramList):paramList_(paramList)
        {
            
            for(uint32_t i=0;i<paramList_.paramList_.size();i++)
            {
                resultList_.push_back(POSResult(paramList_.paramList_[i].mainTag_));
            }

            for(uint32_t i=0;i<resultList_.size();i++)
            {
                tagMap_.insert(resultList_[i].mainTag_, i);
            }
        }
       
        bool getResultIndex(const std::string& pos, uint32_t& index)
        {
            std::string mainTag;
            bool b = paramList_.getMainTag(pos, mainTag);
            if(!b) return false;
            uint32_t *_index = tagMap_.find(mainTag);
            if( _index == NULL) return false;
            index = *_index;
            return true;
        }
        
        void addTermCount(const std::string& term, uint32_t count = 1)
        {
            uint32_t* index = termMap_.find(term);
            if( index == NULL )
            {
                termMap_.insert(term, termList_.size());
                termList_.push_back(std::make_pair(term, count));
                
            }
            else
            {
                termList_[*index].second += count;
            }
        }
        
        void addWordCount(const std::string& word, uint32_t count = 1)
        {
            uint32_t* index = wordMap_.find(word);
            if( index == NULL )
            {
                wordMap_.insert(word, wordList_.size());
                wordList_.push_back(std::make_pair(word, count));
                
            }
            else
            {
                wordList_[*index].second += count;
            }
        }
        
        std::string getString(const std::string& str, uint32_t i)
        {
            izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
            izenelib::util::UString ur;
            ustr.substr(ur, i, 1);
            std::string result;
            ur.convertString(result, izenelib::util::UString::UTF_8);
            return result;
        }
        
        uint32_t getStringLength(const std::string& str)
        {
            izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
            return ustr.length();
        }
        
        void process(const std::vector<std::pair<std::string, std::string> >& itemList)
        {
            for(uint32_t i=0;i<itemList.size()-1;i++)
            {
                uint32_t index1 = 0;
                bool b1 = getResultIndex(itemList[i].second, index1);
                uint32_t index2 = 0;
                bool b2 = getResultIndex(itemList[i+1].second, index2);
                
                if( b1 )
                {
                    std::string chr = getString(itemList[i+1].first,0);
                    resultList_[index1].addSuffix(chr);
                    resultList_[index1].addCount();
                }
                if( b2 )
                {
                    std::string chr = getString(itemList[i].first, getStringLength(itemList[i].first)-1);
//                     std::cout<<"ADD PREFIX "<<chr<<std::endl;
                    resultList_[index2].addPrefix(chr );
                    resultList_[index2].addCount();
                }
                for(uint32_t j=0;j<getStringLength(itemList[i].first);j++)
                {
                    addTermCount( getString( itemList[i].first, j) );
                }
                addWordCount(itemList[i].first);
                
            }
            if( itemList.size() > 0)
            {
                for(uint32_t j=0;j<getStringLength(itemList.back().first);j++)
                {
                    addTermCount( getString( itemList.back().first, j) );
                }
                addWordCount(itemList.back().first);
            }
        }
        
        friend class boost::serialization::access;

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {

            ar  & resultList_ & termList_;
            
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar  & resultList_ & termList_;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

        
        void save(const std::string& file)
        {
//             std::ofstream ofs(file.c_str());
//             {
//                 boost::archive::text_oarchive oa(ofs);
//                 oa << *this;
//             }
//             return;
            for(uint32_t i=0; i<resultList_.size(); i++)
            {
                resultList_[i].sort();
            }
            std::ofstream ofs(file.c_str());
            for(uint32_t p=0;p< resultList_.size();p++)
            {
                POSResult& result = resultList_[p];
                ofs<<result.mainTag_<<":"<<result.allCount_<<std::endl;
                
                for(uint32_t i=0;i<result.prefix_.size();i++)
                {
                    std::pair<std::string, uint32_t>& item = result.prefix_[i];
                    if( item.second>0 )
                        ofs<<item.first<<"|"<<result.mainTag_<<":"<<item.second<<std::endl;
                }
                for(uint32_t i=0;i<result.suffix_.size();i++)
                {
                    std::pair<std::string, uint32_t>& item = result.suffix_[i];
                    if( item.second>0 )
                        ofs<<result.mainTag_<<"|"<<item.first<<":"<<item.second<<std::endl;
                }
            }
            typedef izenelib::util::second_greater<std::pair<std::string, uint32_t> > greater_than;
            std::sort(termList_.begin(), termList_.end(), greater_than());
            for(uint32_t p=0;p< termList_.size();p++)
            {
                
                ofs<<termList_[p].first<<":"<<termList_[p].second<<std::endl;
                
                
            }
            std::cout<<std::endl;
            std::sort(wordList_.begin(), wordList_.end(), greater_than());
            for(uint32_t p=0;p< wordList_.size();p++)
            {
                ofs<<"|word|"<<wordList_[p].first<<":"<<wordList_[p].second<<std::endl;
            }
            ofs.close();
        }
        
    public:
        POSParamList paramList_;
        std::vector<POSResult> resultList_;
        izenelib::am::rde_hash<std::string, uint32_t> tagMap_;
        
        std::vector<std::pair<std::string, uint32_t> > termList_;
        izenelib::am::rde_hash<std::string, uint32_t> termMap_;
        
        std::vector<std::pair<std::string, uint32_t> > wordList_;
        izenelib::am::rde_hash<std::string, uint32_t> wordMap_;
};

// class LabelContextInfo
// {
//     public:
//         POSParamList paramList_;
//         std::vector<POSResult> resultList_;
//         izenelib::am::rde_hash<std::string, uint32_t> tagMap_;
//         
//         std::vector<std::pair<std::string, uint32_t> > termList_;
//         izenelib::am::rde_hash<std::string, uint32_t> termMap_;
//         
//         std::vector<std::pair<std::string, uint32_t> > wordList_;
//         izenelib::am::rde_hash<std::string, uint32_t> wordMap_;
// };
    
class POSUtil
{

public:
   static void dump(const std::string& inputFile, const std::string& outputFile, const POSParamList& paramList,const std::string& splitStr = "_")
   {

        std::ifstream ifs ( inputFile.c_str() );
        std::string word;
        POSResultList* resultList =  new POSResultList(paramList);
        while ( getline ( ifs,word ) )
        {
            
            if( word.length() == 0 ) continue;
            
            izenelib::util::UString ustr(word,izenelib::util::UString::GB2312);
            std::string str;
            ustr.convertString(str, izenelib::util::UString::UTF_8);
//             std::cout<<str<<std::endl;
            std::vector<std::string> splitVec;
            boost::algorithm::split( splitVec, str, boost::algorithm::is_any_of(" ") );
            std::vector<std::pair<std::string, std::string> > itemList;
            BOOST_FOREACH(std::string& item, splitVec)
            {
                boost::algorithm::trim(item);
                if(item.length() == 0) continue;
                std::vector<std::string> wordANDposList;
                boost::algorithm::split( wordANDposList, item, is_any_of(splitStr) );
                if( wordANDposList.size() != 2 )
                {
                    std::cout<<"Error on parsing "<<item<<std::endl;
                }
                else
                {
                    itemList.push_back(std::make_pair(wordANDposList[0], wordANDposList[1]));
//                     std::cout<<wordANDposList[0]<<","<<wordANDposList[1]<<std::endl;
                }
                
            }
            
            resultList->process(itemList);
            
            
        }
        
        ifs.close();
        
        resultList->save(outputFile);
        delete resultList;
   }

POSUtil()
{
    
}

~POSUtil()
{
    
}



private:
    
};

}
#endif

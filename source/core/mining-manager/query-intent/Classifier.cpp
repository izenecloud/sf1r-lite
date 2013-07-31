#include "Classifier.h"
#include <glog/logging.h>

namespace sf1r
{

namespace NQI
{
    const unsigned int REMOVED_WORD_WEIGHT = -1;
    const double RATIO_THRESHOLD = 0.666;
    const std::string NODE_DELIMITER = ">";
    bool nameCompare(WVType& lv, WVType& rv)
    {
        if (REMOVED_WORD_WEIGHT  == lv.second)
            return false;
        else if (REMOVED_WORD_WEIGHT == rv.second)
            return true;
        else
            return lv.first < rv.first;
    }

    bool weightCompare(WVType& lv, WVType& rv)
    {
        return lv.second > rv.second;
    }

    bool calculateWeight(WVType& lv, WVType& rv)
    {
        if (std::string::npos == lv.first.find(rv.first))
            return false;
        lv.second += rv.second;
        return true;
    }

    bool calculateWeight(WVType& wv, ValueType& v)
    {
        if (std::string::npos == wv.first.find(v))
            return false;
        wv.second ++;
        return true;
    }

    bool combineWMV(WMVType& wmv, int number)
    {
        wmv.sort(nameCompare);
        // 
        WMVIterator it = wmv.begin();
        for (; it != wmv.end(); it++)
        {
            WMVIterator next = it;
            next++;
            if (wmv.end() == next)
                break;
            if (it->first == next->first)
            {
                next->second += it->second;
                wmv.erase(it);
            }
        }
        
        for (it = wmv.begin(); it != wmv.end(); it++)
        {
            WVType& value = *it;
            WMVIterator cIt = it;
            cIt ++;
            bool ret = false;
            for (; cIt != wmv.end(); cIt++)
            {
                ret |= calculateWeight(*cIt, value); 
            }
            //std::cout<<it->first<<" "<<it->second<<"\n";
            if (ret)
                wmv.erase(it);
        }
        wmv.sort(weightCompare);

        if (-1 == number || number >= (int)wmv.size())
            return true;
        
        unsigned int sumVote = 0;
        unsigned int sumRest = 0;
        number -= 1;
        for (it = wmv.begin(); it != wmv.end(); it++)
        {
            if (it->second == REMOVED_WORD_WEIGHT)
                continue;
            if (number >= 0)
            {
                number--;
                sumVote += it->second;
                continue;
            }
            sumRest += it->second;
        }
        double ratio = (double) sumVote / (sumVote + sumRest);
        if (ratio > RATIO_THRESHOLD)
            return true;
        return false;
    }

    bool commonAncester(WMVType& wmv, int number)
    {
        //printWMV(wmv);
        wmv.sort(nameCompare);
        WMVIterator it = wmv.begin();
        if (REMOVED_WORD_WEIGHT == it->second)
            it++;
        for (; it != wmv.end(); it++)
        {
            std::queue<std::size_t> queue;
            std::queue<std::size_t> nextQueue;
            
            WMVIterator next = it;
            next++;
            if (wmv.end() == next)
                break;
            if (it->first == next->first || std::string::npos != next->first.find(it->first))
            {
                it->second += next->second;
                wmv.erase(next);
                continue;
            }
            else if (std::string::npos != it->first.find(next->first))
            {
                next->second += it->second;
                wmv.erase(it);
                continue;
            }
            
            std::size_t pos = it->first.size() - 1;
            while (true)
            {
                std::size_t found = it->first.find_last_of(NODE_DELIMITER, pos);
                if (std::string::npos == found)
                    break;
                queue.push(found);
                pos = found - 1;
            }
            
            pos = next->first.size() - 1;
            while (true)
            {
                std::size_t found = next->first.find_last_of(NODE_DELIMITER, pos);
                if (std::string::npos == found)
                    break;
                nextQueue.push(found);
                pos = found - 1;
            }

            while (true)
            {
                if (queue.empty() || nextQueue.empty())
                    break;
                if (queue.size() > nextQueue.size())
                {
                    queue.pop();
                    continue;
                }
                if (queue.size() < nextQueue.size())
                {
                    nextQueue.pop();
                    continue;
                }
                std::string lv = it->first.substr(0, queue.front());
                queue.pop();
                std::string rv = next->first.substr(0, nextQueue.front());
                nextQueue.pop();
                if (lv == rv)
                {
                    it->first = lv;
                    it->second += next->second;
                    wmv.erase(next);
                    break;
                }
            }
        }
        
        if (-1 == number || number >= (int)wmv.size())
            return true;
        
        unsigned int sumVote = 0;
        unsigned int sumRest = 0;
        number -= 1;
        for (it = wmv.begin(); it != wmv.end(); it++)
        {
            if (it->second == REMOVED_WORD_WEIGHT)
                continue;
            if (number >= 0)
            {
                number--;
                sumVote += it->second;
                continue;
            }
            sumRest += it->second;
        }
        double ratio = (double) sumVote / (sumVote + sumRest);
        if (ratio > RATIO_THRESHOLD)
            return true;
        return false;
    }
    
    void combineWMVS(WMVContainer& wmvs, boost::unordered_map<std::string, std::list<std::string> >& scs, std::string& remainKeywords)
    {
        //std::cout<<remainKeywords<<"\n";
        WMVCIterator it = wmvs.begin();
        for (; it != wmvs.end(); it++)
        {
            //printWMV(it->second);
            if (!combineWMV(it->second, it->first.operands_))
            {
                resumeKeywords(it->second, remainKeywords);
                deleteKeywords(it->second);
                //printWMV(it->second);
                if (!commonAncester(it->second, it->first.operands_))
                {
                    wmvs.erase(it);
                }
            }
            else
            {
                boost::unordered_map<std::string, std::list<std::string> >::iterator sIt = scs.find(it->first.name_);
                if ( scs.end() == sIt )
                {
                    deleteKeywords(it->second);
                    continue;
                }
                WMVIterator wmv = it->second.begin();
                for (; wmv != it->second.end(); wmv++)
                {
                    if (REMOVED_WORD_WEIGHT  == wmv->second)
                        continue;
                    std::list<std::string>::iterator svIt = sIt->second.begin();
                    for (; svIt != sIt->second.end(); svIt++)
                    {
                        //LOG(INFO)<<*svIt<<" "<<wmv->first;
                        if ((*svIt) == wmv->first)
                            break;
                    }

                    if (sIt->second.end() == svIt)
                    {
                        resumeKeywords(it->second, remainKeywords);
                        break;
                    }
                }
                wmvs.erase(it);
            }
        }
        //std::cout<<remainKeywords<<"\n";
    }
    
    void printWMV(WMVType& wmv)
    {
        std::cout<<"Print begin\n";
        WMVIterator it = wmv.begin();
        for (; it != wmv.end(); it++)
        {
            std::cout<<it->first<<" "<<it->second<<"\n";
        }
        std::cout<<"Print end\n";
    }

    void deleteKeywords(WMVType& wmv)
    {
        wmv.sort(weightCompare);
        WMVIterator it = wmv.begin();
        if (REMOVED_WORD_WEIGHT == it->second)
        {
            wmv.erase(it);
        }
    }

    void reserveKeywords(WMVType& wmv, std::string& removedWords)
    {
        if (removedWords.empty())
            return;
        wmv.sort(weightCompare);
        WMVIterator it = wmv.begin();
        if (REMOVED_WORD_WEIGHT == it->second)
        {
            it->first += " ";
            it->first += removedWords;
        }
        else
            wmv.push_front(make_pair(removedWords, REMOVED_WORD_WEIGHT));
    }

    void resumeKeywords(WMVType& wmv, std::string& keyword)
    {
        wmv.sort(weightCompare);
        WMVIterator it = wmv.begin();
        if (REMOVED_WORD_WEIGHT == it->second)
        {
            keyword += " ";
            keyword += it->first;
        }
    }
}

}

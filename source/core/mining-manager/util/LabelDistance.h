/*
 * @file  LabelEditDistance.h
 * @author  Jinglei
 * @date 2010.01.02
  */

#ifndef LABELDISTANCE_H_
#define LABELDISTANCE_H_

#include <list>
#include <mining-manager/taxonomy-generation-submanager/TgTypes.h>
#include <util/ustring/algo.hpp>
#include <vector>
#include <algorithm>

namespace sf1r{

class LabelDistance
{
public:
    LabelDistance(){}
    virtual ~LabelDistance() {}

    /**
     * @ brief check a label's uniqueness against a lexicon.
     */
    static inline bool isDuplicate(
            const TgLabelInfo& label,
            const std::list< TgLabelInfo >& lexicon)
    {

        if(!label.termList_.size()||!label.termList_[0].length())
            return true;
        if(label.termList_[0].isChineseChar(0))
            return  isChineseDuplicate(label,lexicon);

        std::list< TgLabelInfo >::const_iterator it=lexicon.begin();
        std::list< TgLabelInfo >::const_iterator itEnd= lexicon.end();
        for(;it!=itEnd;it++)
        {
            if(it->termList_.size()!=label.termList_.size()
                    ||!isFirstCharacterEqual(it->termList_, label.termList_))
                continue;
            float dis=LabelDistance::getDistance(it->name_,label.name_);
            if(dis<4)
            {
                return true;
            }

        }
        return false;
    }

    /**
     * @ brief check a label's uniqueness against a lexicon and a given query.
     */
    static inline bool isDuplicate(
            const TgLabelInfo& label,
            const std::vector<izenelib::util::UString>& normalizedQueryTermList,
            const izenelib::util::UString& queryStr)
    {

        if(!label.termList_.size()||!label.termList_[0].length())
            return true;
        if(label.termList_[0].isChineseChar(0))
        {
            if(label.name_==queryStr)
            {
                std::cout<<"The query and the label is equal!"<<std::endl;
                return true;
            }
            else if(label.termList_.size()<queryStr.length())
            {
                if(queryStr.find(label.name_)!=izenelib::util::UString::NOT_FOUND)
                    return true;
            }
            else if(label.termList_.size()-queryStr.length()==1)
            {
                if(getDistance(label.name_, queryStr)==1)
                    return true;
            }
            else
            {
                return (isChineseDuplicate(label.termList_,normalizedQueryTermList));
            }

        }

        if(label.termList_.size()!=normalizedQueryTermList.size()
                ||!isFirstCharacterEqual(label.termList_, normalizedQueryTermList))
            return false;
        float dis=LabelDistance::getDistance(label.name_,queryStr);
        if(dis<4)
        {
            return true;
        }
        return false;

    }

    /**
     * @brief used for de-duplicate results for English auto completion.
     */
    static inline bool isDuplicate(
            const izenelib::util::UString& label,
            const std::vector<izenelib::util::UString>& lexicon)
    {

        if(!label.length())
            return true;
        if(label.isChineseChar(0))
            return false;
        for(unsigned int i=0;i<lexicon.size();i++)
        {
            float dis=getDistance(label,lexicon[i]);
            //The distance could be defined to be more flexible.
            if(dis==0)
            {
                return true;
            }
        }
        return false;

    }

    /**
     * @ brief check a label's uniqueness against a lexicon for Chinese.
     */
    static inline bool isChineseDuplicate(
            const TgLabelInfo& label,
            const std::list< TgLabelInfo >& lexicon
            )
    {
         std::list< TgLabelInfo >::const_iterator it=lexicon.begin();
         std::list< TgLabelInfo >::const_iterator itEnd= lexicon.end();
         for(;it!=itEnd;it++)
         {
             bool ret=isChineseDuplicate(label, *it);
             if(ret)
             {
                 return true;
             }
         }
         return false;
    }

    /**
     * @ brief check whether two Chinese labels are duplicates of each other.
     */
    static inline bool isChineseDuplicate(
            const TgLabelInfo& label1,
            const TgLabelInfo& label2)
    {

        if(label1.termList_.size()<label2.termList_.size())
        {
            if(getDistance(label1.name_, label2.name_)<2)
                return true;
        }
        else if(label1.termList_.size()<label2.termList_.size())
        {
            if(label2.name_.find(label1.name_)!=izenelib::util::UString::NOT_FOUND)
                return true;
        }
        return isChineseDuplicate(label1.termList_, label2.termList_);

    }

    /**
     * @ brief check whether two Chinese labels are duplicates of each other.
     */
    static inline bool isChineseDuplicate(
            const std::vector<izenelib::util::UString>& label1,
            const std::vector<izenelib::util::UString>& label2)
    {
        if(label1.size()==0||label2.size()==0)
        {
            return false;
        }
        if(label1.size()>2&&label2.size()>2
                &&label1[0]==label2[0]
                &&label1[label1.size()-1]==label2[label2.size()-1]
           )
        {
            return true;
        }
        if(label1.size()<=label2.size())
        {
            std::vector<izenelib::util::UString> tempLabel1(label1.begin(), label1.end());
            std::sort(tempLabel1.begin(), tempLabel1.end());
            std::vector<izenelib::util::UString> tempLabel2(label2.begin(), label2.end());
            std::sort(tempLabel2.begin(), tempLabel2.end());
            izenelib::util::UString tempStr1, tempStr2;
            for(uint32_t i=0;i<tempLabel1.size();i++)
            {
                tempStr1+=tempLabel1[i];
                tempStr2+=tempLabel2[i];
            }
            if(getDistance(tempStr1, tempStr2)==0||(tempStr1.length()>2&&getDistance(tempStr1, tempStr2)<2))
            {
                return true;
            }
        }
        return false;

    }


    /**
     * @ brief Compute UString-based edit distance.
     */
    static inline float getDistance(
            const izenelib::util::UString& s1,
            const izenelib::util::UString& s2)
    {
        izenelib::util::UString ls1(s1);
        izenelib::util::UString ls2(s2);
        ls1.toLowerString();
        ls2.toLowerString();
        const unsigned int HEIGHT = ls1.length() + 1;
        const unsigned int WIDTH = ls2.length() + 1;
        unsigned int eArray[HEIGHT][WIDTH];
        unsigned int i;
        unsigned int j;

        for (i = 0; i < HEIGHT; i++)
            eArray[i][0] = i;

        for (j = 0; j < WIDTH; j++)
            eArray[0][j] = j;

        for (i = 1; i < HEIGHT; i++) {
            for (j = 1; j < WIDTH; j++) {
                eArray[i][j] = min(
                        eArray[i - 1][j - 1] +
                            (ls1[i-1] == ls2[j-1] ? 0 : 1),
                        min(eArray[i - 1][j] + 1, eArray[i][j - 1] + 1));
            }
        }

       return eArray[HEIGHT - 1][WIDTH - 1];

    }

    /**
     * @ brief Do pre-filtering for English label de-duplication.
     */
    static inline bool isFirstCharacterEqual(
            const std::vector<izenelib::util::UString>& label1,
            const std::vector<izenelib::util::UString>& label2
            )
    {
        assert(label1.size()==label2.size());

        for(unsigned int i=0;i<label1.size();i++)
        {

            if(!label1[i].length()||!label2[i].length())
                return false;
            if(!is_alphabet<uint16_t>::equal_ignore_case(label1[i].at(0),label2[i].at(0)))
                return false;
        }
        return true;

    }

private:


};


}

#endif /* LABELDISTANCE_H_ */

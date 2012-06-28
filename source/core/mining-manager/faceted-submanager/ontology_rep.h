///
/// @file ontology-rep.h
/// @brief ontology representation class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_REP_H_
#define SF1R_ONTOLOGY_REP_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_rep_item.h"
#include <util/PriorityQueue.h>

NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class OntologyRep
{
public:
    typedef std::list<OntologyRepItem>::iterator item_iterator;
    /** attribute name , and its doc count */
    typedef std::pair<sf1r::faceted::CategoryNameType, int> AttrNameFreq;

    class AttrNameFreqQueue : public izenelib::util::PriorityQueue<AttrNameFreq>
    {
    public:
        AttrNameFreqQueue(size_t size)
        {
            this->initialize(size);
        }

    protected:
        bool lessThan(const AttrNameFreq& p1, const AttrNameFreq& p2) const
        {
            return (p1.second < p2.second);
        }
    };


    OntologyRep() {}
    ~OntologyRep() {}

    std::list<OntologyRepItem> item_list;

    item_iterator Begin()
    {
        return item_list.begin();
    }

    item_iterator End()
    {
        return item_list.end();
    }

    void swap(OntologyRep& rep)
    {
        item_list.swap(rep.item_list);
    }

    /* -----------------------------------*/
    /**
     * @Brief  merge the OntologyRep and keep the top topGroupNum number by doc_count.
     * Make sure the list of OntologyRep with level=0(that means attribute name) 
     * is in order by doc_count before merge and level=1 (that means attribute value)
     *  is in order by value id before merge.
     *
     * @Param topGroupNum
     * @Param other
     */
    /* -----------------------------------*/
    void merge(int topGroupNum, std::list<OntologyRep*>& others)
    {
        std::list<OntologyRep*>::iterator others_it = others.begin();
        while(others_it != others.end())
        {
            if((*others_it)->item_list.size() > 0)
            {
                if (item_list.empty())
                {
                    // find the first non-empty to swap with self
                    swap(*(*others_it));
                    others_it = others.erase(others_it);
                }
                else
                    ++others_it;
            }
            else
            {
                // clear empty OntologyRep
                others_it = others.erase(others_it);
            }
        }
        if(others.size() == 0)
        {
            if(topGroupNum == 0 || ((int)item_list.size() <= topGroupNum) )
                return;
            item_iterator it = item_list.begin();
            int group_num = 0;
            while(it != item_list.end())
            {
                if((*it).level == 0)
                    ++group_num;
                if( group_num > topGroupNum )
                {
                    break;
                }
                ++it;
            }
            item_list.erase(it, item_list.end());
            return;
        }
        others_it = others.begin();

        typedef std::list<OntologyRepItem> ValueCountList;
        typedef std::map<CategoryNameType, int> NameCountMap;
        typedef std::map<CategoryNameType, ValueCountList> NameValueCountMap;
        NameValueCountMap nv_cnt_map;
        NameCountMap name_cnt_map;
        item_iterator it = item_list.begin();
        CategoryNameType current_name;
        NameValueCountMap::iterator current_nvmap_it; 
        while(it != item_list.end())
        {
            if((*it).level == 0)
            {
                current_name = (*it).text;
                name_cnt_map[current_name] = (*it).doc_count;
                current_nvmap_it = nv_cnt_map.insert(std::make_pair(current_name, ValueCountList())).first;
            }
            else
            {
                (current_nvmap_it->second).push_back(*it);
            }
            ++it;
        }
        while(others_it != others.end())
        {
            OntologyRep& other = *(*others_it);
            it = other.item_list.begin();
            bool newname = false;
            ValueCountList::iterator current_value_it;
            while(it != other.item_list.end())
            {
                if((*it).level == 0)
                {
                    current_name = (*it).text;
                    std::pair<NameCountMap::iterator, bool> n_it = name_cnt_map.insert(std::make_pair(current_name, 0));
                    n_it.first->second += it->doc_count;
                    // whether a new name added
                    newname = n_it.second;
                    current_nvmap_it = nv_cnt_map.insert(std::make_pair(current_name, ValueCountList())).first;
                    current_value_it = current_nvmap_it->second.begin();
                }
                else
                {
                    if(newname)
                    {
                        (current_nvmap_it->second).push_back(*it);
                    }
                    else
                    {
                        while(current_value_it->id < it->id)
                            ++current_value_it;
                        if(current_value_it->id == it->id)
                        {
                            current_value_it->doc_count += it->doc_count;
                        }
                        else
                        {
                            current_nvmap_it->second.insert(current_value_it, *it);
                        }
                    }
                }
                ++it;
            }
            ++others_it;
        }
        if (topGroupNum == 0)
        {
            topGroupNum = name_cnt_map.size();
        }
        AttrNameFreqQueue queue(topGroupNum);
        NameCountMap::const_iterator cit = name_cnt_map.begin();
        while (cit != name_cnt_map.end())
        {
            queue.insert(AttrNameFreq(cit->first, cit->second));
            ++cit;
        }
        std::size_t retsize = queue.size();
        std::vector<CategoryNameType> top_names;
        top_names.resize(retsize);
        for(int i = (int)retsize - 1; i >= 0; --i)
        {
            top_names[i] = queue.pop().first;
        }

        item_iterator item_nextit = item_list.begin();
        for (std::size_t i = 0; i < retsize; ++i)
        {
            CategoryNameType name = top_names[i];

            if(item_nextit == item_list.end())
            {
                item_nextit = item_list.insert(item_nextit, OntologyRepItem());
            }
            OntologyRepItem& item = *item_nextit;
            item.level = 0;
            item.text = name;
            item.doc_count = name_cnt_map[name];
            const ValueCountList& value_cnt_map = nv_cnt_map[name];
            ValueCountList::const_iterator cit = value_cnt_map.begin();
            while(cit != value_cnt_map.end())
            {
                ++item_nextit;
                if(item_nextit == item_list.end())
                {
                    item_nextit = item_list.insert(item_nextit, OntologyRepItem());
                }
                OntologyRepItem& sub_item = *item_nextit;
                sub_item.level = 1;
                sub_item.id = cit->id;
                sub_item.text = cit->text;
                sub_item.doc_count = cit->doc_count;
                ++cit;
            }
            ++item_nextit;
        }
        item_list.erase(item_nextit, item_list.end());
    }

    bool operator==(const OntologyRep& rep) const
    {
        if (item_list.size()!=rep.item_list.size()) return false;
        std::list<OntologyRepItem>::const_iterator it1 = item_list.begin();
        std::list<OntologyRepItem>::const_iterator it2 = rep.item_list.begin();
        while (it1!=item_list.end() && it2!=rep.item_list.end() )
        {
            if (!(*it1==*it2)) return false;
            ++it1;
            ++it2;
        }
        return true;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        std::list<OntologyRepItem>::const_iterator it = item_list.begin();
        while (it!=item_list.end())
        {
            OntologyRepItem item = *it;
            ++it;
            std::string str;
            item.text.convertString(str, izenelib::util::UString::UTF_8);
            ss<<(int)item.level<<","<<item.id<<","<<str<<","<<item.doc_count<<std::endl;
        }
        return ss.str();
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & item_list;
    }

    MSGPACK_DEFINE(item_list);
};
NS_FACETED_END
#endif /* SF1R_ONTOLOGY_REP_H_ */

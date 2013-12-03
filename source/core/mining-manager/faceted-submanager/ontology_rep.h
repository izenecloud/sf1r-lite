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
    typedef std::list<OntologyRepItem>::const_iterator const_item_iterator;
    /** attribute name, and its score */
    typedef std::pair<sf1r::faceted::CategoryNameType, double> AttrNameScore;

    class AttrNameScoreQueue : public izenelib::util::PriorityQueue<AttrNameScore>
    {
    public:
        AttrNameScoreQueue(size_t size)
        {
            this->initialize(size);
        }

    protected:
        bool lessThan(const AttrNameScore& p1, const AttrNameScore& p2) const
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
     * @Param topGroupNum
     * @Param other
     * @post the merged attr names are sorted by name score in descending order
     * @post the merged attr values are sorted by value score in descending order
     */
    /* -----------------------------------*/
    void merge(int topGroupNum, std::list<const OntologyRep*>& others)
    {
        std::list<const OntologyRep*>::iterator others_it = others.begin();
        while(others_it != others.end())
        {
            if((*others_it)->item_list.size() > 0)
            {
                if (item_list.empty())
                {
                    // find the first non-empty to swap with self
                    item_list = (*(*others_it)).item_list;
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

        typedef std::map<CategoryNameType, OntologyRepItem> NameItemMap;
        NameItemMap name_item_map;

        typedef std::map<CategoryNameType, OntologyRepItem> ValueItemMap;
        typedef std::map<CategoryNameType, ValueItemMap> NameValueItemMap;
        NameValueItemMap nv_item_map;

        const_item_iterator it = item_list.begin();
        CategoryNameType current_name;
        NameValueItemMap::iterator current_nvmap_it;
        while(it != item_list.end())
        {
            if((*it).level == 0)
            {
                current_name = (*it).text;
                name_item_map[current_name] = *it;
                current_nvmap_it = nv_item_map.insert(std::make_pair(current_name, ValueItemMap())).first;
            }
            else
            {
                (current_nvmap_it->second)[it->text] = (*it);
            }
            ++it;
        }
        while(others_it != others.end())
        {
            const OntologyRep& other = *(*others_it);
            it = other.item_list.begin();
            bool newname = false;
            ValueItemMap::iterator current_value_it;
            while(it != other.item_list.end())
            {
                if((*it).level == 0)
                {
                    current_name = (*it).text;
                    std::pair<NameItemMap::iterator, bool> n_it = name_item_map.insert(std::make_pair(current_name, *it));
                    if (!n_it.second)
                    {
                        n_it.first->second.doc_count += it->doc_count;
                        n_it.first->second.score += it->score;
                    }
                    // whether a new name added
                    newname = n_it.second;
                    current_nvmap_it = nv_item_map.insert(std::make_pair(current_name, ValueItemMap())).first;
                }
                else
                {
                    if(newname)
                    {
                        (current_nvmap_it->second)[it->text] = *it;
                    }
                    else
                    {
                        current_value_it = current_nvmap_it->second.find(it->text);

                        if(current_value_it != current_nvmap_it->second.end())
                        {
                            current_value_it->second.doc_count += it->doc_count;
                            current_value_it->second.score += it->score;
                        }
                        else
                        {
                            (current_nvmap_it->second)[it->text] = (*it);
                        }
                    }
                }
                ++it;
            }
            ++others_it;
        }
        if (topGroupNum == 0)
        {
            topGroupNum = name_item_map.size();
        }
        AttrNameScoreQueue queue(topGroupNum);
        for (NameItemMap::const_iterator n_it = name_item_map.begin();
             n_it != name_item_map.end(); ++n_it)
        {
            queue.insert(AttrNameScore(n_it->second.text, n_it->second.score));
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
            const CategoryNameType& name = top_names[i];

            if(item_nextit == item_list.end())
            {
                item_nextit = item_list.insert(item_nextit, OntologyRepItem());
            }
            *item_nextit = name_item_map[name];
            ++item_nextit;

            typedef std::multimap<double, OntologyRepItem> ScoreItemMap;
            ScoreItemMap score_item_map;

            const ValueItemMap& value_item_map = nv_item_map[name];
            for (ValueItemMap::const_iterator vit = value_item_map.begin();
                 vit != value_item_map.end(); ++vit)
            {
                score_item_map.insert(ScoreItemMap::value_type(vit->second.score,
                                                               vit->second));
            }

            // sort by value score in descending order
            for (ScoreItemMap::const_reverse_iterator sit = score_item_map.rbegin();
                 sit != score_item_map.rend(); ++sit)
            {
                if(item_nextit == item_list.end())
                {
                    item_nextit = item_list.insert(item_nextit, OntologyRepItem());
                }
                *item_nextit = sit->second;
                ++item_nextit;
            }
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
        for (std::list<OntologyRepItem>::const_iterator it = item_list.begin();
             it!=item_list.end(); ++it)
        {
            const OntologyRepItem& item = *it;
            std::string str;
            item.text.convertString(str, izenelib::util::UString::UTF_8);
            ss << (int)item.level << ","
               << item.id << ","
               << str << ","
               << item.doc_count << ","
               << item.score << std::endl;
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

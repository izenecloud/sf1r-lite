#ifndef SF1R_B5MMANAGER_TUANCLUSTERING_H
#define SF1R_B5MMANAGER_TUANCLUSTERING_H 
#include <util/ustring/UString.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <product-manager/product_price.h>
#include "b5m_helper.h"
#include "b5m_types.h"
#include "b5m_m.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <common/PairwiseScdMerger.h>
NS_SF1R_B5M_BEGIN

class TuanClustering{
    typedef uint32_t term_t;
    typedef std::vector<term_t> Sentence;
    struct NodeValue
    {
        Sentence label;
        std::vector<Document> docs;
        double score;
        NodeValue& operator+=(const NodeValue& value)
        {
            docs.insert(docs.end(), value.docs.begin(), value.docs.end());
            return *this;
        }
        void DocsUnique()//unique the docs
        {
            std::sort(docs.begin(), docs.end(), DocidCompare); 
            docs.erase(std::unique(docs.begin(), docs.end(), DocidEqual), docs.end());
        }
        bool operator<(const NodeValue& value) const
        {
            return score>value.score;
        }

        static bool DocidCompare(const Document& doc1, const Document& doc2)
        {
            return doc1.getId()<doc2.getId();
        }
        static bool DocidEqual(const Document& doc1, const Document& doc2)
        {
            return doc1.getId()==doc2.getId();
        }
    };
    typedef std::map<Sentence, NodeValue> Tree;
    typedef izenelib::util::UString UString;

    typedef std::string DocIdType;

public:
    TuanClustering(idmlib::util::IDMAnalyzer* analyzer): analyzer_(analyzer), n_(0)
    {
    }
    ~TuanClustering()
    {
    }
    void InsertDoc(const Document& doc)
    {
        Sentence sentence;
        UString title;
        doc.getString("Title", title);
        std::vector<idmlib::util::IDMTerm> term_list;
        analyzer_->GetTermList(title, term_list);
        for (uint32_t i=0;i<term_list.size();i++)
        {
            const UString& ustr = term_list[i].text;
            term_t term = izenelib::util::HashFunction<UString>::generateHash32(ustr);
            sentence.push_back(term);
            std::string str;
            ustr.convertString(str, UString::UTF_8);
            term2str_[term] = str;
            //std::cerr<<"find term "<<str<<" : "<<term<<std::endl;
        }
        NodeValue value;
        value.docs.push_back(doc);
        value.docs.front().setId(++n_);
        Insert_(sentence, value);
    }
    void Build()
    {
        std::vector<NodeValue> clusters;
        boost::unordered_set<Sentence> app;
        for(Tree::iterator it = tree_.begin();it!=tree_.end();++it)
        {
            const Sentence& s = it->first;
            for(std::size_t i=2;i<s.size();i++)
            {
                Sentence frag(s.begin(), s.begin()+i);
                if(app.find(frag)!=app.end()) continue;
                app.insert(frag);
                NodeValue value;
                value.label = frag;
                Tree::iterator it2 = it;
                for(;it2!=tree_.end();++it2)
                {
                    const Sentence& s2 = it2->first;
                    NodeValue& value2 = it2->second;
                    if(boost::algorithm::starts_with(s2, frag))
                    {
                        value+=value2;
                    }
                    else
                    {
                        break;
                    }
                }
                value.DocsUnique();
                ComputeScore_(value);
                if(value.docs.size()<2) continue;
                clusters.push_back(value);
            }
        }
        std::sort(clusters.begin(), clusters.end());
        typedef boost::unordered_map<std::size_t, std::size_t> MergeMap;
        MergeMap merge_map;
        for(std::size_t i=0;i<clusters.size();i++)
        {
            for(std::size_t j=i+1;j<clusters.size();j++)
            {
                if(CanMerge_(clusters[i], clusters[j]))
                {
                    std::size_t index = i;
                    MergeMap::const_iterator it = merge_map.find(i);
                    if(it!=merge_map.end())
                    {
                        index = it->second;
                    }
                    merge_map[i] = index;
                    merge_map[j] = index;
                }
            }
        }
        typedef boost::unordered_map<std::size_t, std::vector<std::size_t> > FinalIndexes;
        FinalIndexes final_indexes;
        for(MergeMap::const_iterator it=merge_map.begin();it!=merge_map.end();++it)
        {
            final_indexes[it->second].push_back(it->first);
        }
        for(std::size_t i=0;i<clusters.size();i++)
        {
            NodeDebug_(clusters[i]);
        }
        std::vector<NodeValue> merged_clusters;
        for(FinalIndexes::const_iterator it=final_indexes.begin();it!=final_indexes.end();++it)
        {
            std::size_t index = it->first;
            const std::vector<std::size_t>& indexes = it->second;
            NodeValue& value = clusters[index];
            for(std::size_t i=0;i<indexes.size();i++)
            {
                value+=clusters[indexes[i]];
            }
            value.DocsUnique();
            ComputeScore_(value);
            merged_clusters.push_back(value);
            std::cerr<<"\tFINAL\t";
            NodeDebug_(value);
        }
        typedef std::vector<std::vector<double> > Matrix;
        Matrix matrix(n_+1, std::vector<double>(n_+1, 0.0));
        std::vector<Document> docs(n_+1);
        for(std::size_t i=0;i<merged_clusters.size();i++)
        {
            const NodeValue& cluster = merged_clusters[i];
            for(std::size_t j=0;j<cluster.docs.size();j++)
            {
                docs[cluster.docs[j].getId()] = cluster.docs[j];
                for(std::size_t k=j+1;k<cluster.docs.size();k++)
                {
                    uint32_t docid1 = cluster.docs[j].getId();
                    uint32_t docid2 = cluster.docs[k].getId();
                    if(docid1>docid2) std::swap(docid1, docid2);
                    matrix[docid1][docid2] += cluster.score;
                }
            }
        }
        typedef std::pair<double, std::pair<uint32_t, uint32_t> > OrderScore;
        std::vector<OrderScore> order_scores;
        for(uint32_t i=1;i<matrix.size();i++)
        {
            for(uint32_t j=i+1;j<matrix.size();j++)
            {
                double score = matrix[i][j];
                const Document& doc1 = docs[i];
                const Document& doc2 = docs[j];
                double p1 = ProductPrice::ParseDocPrice(doc1);
                double p2 = ProductPrice::ParseDocPrice(doc2);
                if(p1<=0.0||p2<=0.0) score = 0.0;
                else
                {
                    double small = std::min(p1, p2);
                    double large = std::max(p1, p2);
                    score *= (small/large);
                }
                matrix[i][j] = score;
                if(score>0.0)
                {
                    OrderScore os(score, std::make_pair(i, j));
                    order_scores.push_back(os);
                }
            }
        }
        std::sort(order_scores.begin(), order_scores.end(), std::greater<OrderScore>());
        typedef boost::unordered_map<uint32_t, uint32_t> Result;
        Result result;
        uint32_t iresult = 0;
        for(std::size_t i=0;i<order_scores.size();i++)
        {
            uint32_t i1 = order_scores[i].second.first;
            uint32_t i2 = order_scores[i].second.second;
            const Document& doc1 = docs[i1];
            const Document& doc2 = docs[i2];
            double score = order_scores[i].first;
            std::string title1;
            std::string title2;
            doc1.getString("Title", title1);
            doc2.getString("Title", title2);
            std::cerr<<"\t\t Order : "<<score<<" , "<<title1<<"\t|\t"<<title2<<std::endl;
            if(score<10.0) continue;
            Result::const_iterator it1 = result.find(i1);
            Result::const_iterator it2 = result.find(i2);
            if(it1==result.end()&&it2==result.end())
            {
                result.insert(std::make_pair(i1, iresult));
                result.insert(std::make_pair(i2, iresult));
                iresult++;
            }
            else if(it1==result.end()&&it2!=result.end())
            {
                result.insert(std::make_pair(i1, it2->second));
            }
            else if(it1!=result.end()&&it2==result.end())
            {
                result.insert(std::make_pair(i2, it1->second));
            }
        }
        FinalIndexes final;
        for(Result::const_iterator it=result.begin();it!=result.end();++it)
        {
            final[it->second].push_back(it->first);
        }
        for(FinalIndexes::const_iterator it=final.begin();it!=final.end();++it)
        {
            std::cerr<<"Cluster "<<it->first<<" : "<<std::endl;
            for(std::size_t i=0;i<it->second.size();i++)
            {
                const Document& doc = docs[it->second[i]];
                std::string title;
                doc.getString("Title", title);
                std::cerr<<"\t"<<title<<std::endl;
            }
        }

    }

    void NodeDebug_(const NodeValue& value)
    {
        const Sentence& s = value.label;
        //std::cerr<<"label length "<<s.size()<<std::endl;
        for(std::size_t i=0;i<s.size();i++)
        {
            std::cerr<<term2str_[s[i]];    
        }
        std::cerr<<" , documents "<<value.docs.size()<<" , score "<<value.score<<std::endl;
    }


private:

    bool CanMerge_(const NodeValue& v1, const NodeValue& v2) const
    {
        double k = 0.5;
        std::size_t mixed_count = 0;
        std::size_t i=0;
        std::size_t j=0;
        while(i<v1.docs.size()&&j<v2.docs.size())
        {
            if(v1.docs[i].getId()<v2.docs[j].getId())
            {
                ++i;
            }
            else if(v1.docs[i].getId()>v2.docs[j].getId())
            {
                ++j;
            }
            else
            {
                mixed_count++;
                ++i;
                ++j;
            }
        }
        double k1 = (double)mixed_count/v1.docs.size();
        double k2 = (double)mixed_count/v2.docs.size();
        if(k1>k&&k2>k) return true;
        return false;
    }


    void Insert_(const Sentence& sentence, NodeValue& value)
    {
        for(std::size_t i=0;i<sentence.size();i++)
        {
            Sentence frag(sentence.begin()+i, sentence.end());
            if(frag.size()>7||frag.size()<2) continue;
            value.label = frag;
            Tree::iterator it = tree_.find(frag);
            if(it==tree_.end())
            {
                tree_.insert(std::make_pair(frag, value));
            }
            else
            {
                it->second+=value;
            }
        }
    }

    void ComputeScore_(NodeValue& value)
    {
        value.score = (double)value.docs.size()*value.label.size();
    }

private:
    idmlib::util::IDMAnalyzer* analyzer_;
    std::size_t n_;
    Tree tree_;
    std::map<uint32_t, std::string> term2str_;
};

NS_SF1R_B5M_END

#endif


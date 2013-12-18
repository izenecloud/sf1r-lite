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
    typedef boost::unordered_map<std::string, std::string> MatchResult;

public:
    TuanClustering(idmlib::util::IDMAnalyzer* analyzer): analyzer_(analyzer), n_(0), max_cluster_size_(0)
    {
    }
    ~TuanClustering()
    {
    }
    std::size_t MaxClusterSize() const
    {
        return max_cluster_size_;
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
    void Build(MatchResult& mresult)
    {
        std::vector<NodeValue> clusters;
        boost::unordered_set<Sentence> app;
        for(Tree::iterator it = tree_.begin();it!=tree_.end();++it)
        {
            const Sentence& s = it->first;
            for(std::size_t i=2;i<s.size();i++)
            {
                Sentence frag(s.begin(), s.begin()+i);
                if(frag.size()>7 || frag.size()<3) continue;
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
        //for(std::size_t i=0;i<clusters.size();i++)
        //{
        //    NodeDebug_(clusters[i]);
        //}
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
            //std::cerr<<"\tFINAL\t";
            //NodeDebug_(value);
        }
        std::sort(merged_clusters.begin(), merged_clusters.end());
        for(std::size_t i=0;i<merged_clusters.size();i++)
        {
            const NodeValue& c1 = merged_clusters[i];
            if(c1.docs.empty()) continue;
            for(std::size_t j=i+1;j<merged_clusters.size();j++)
            {
                NodeValue& c2 = merged_clusters[j]; 
                if(boost::algorithm::contains(c1.label, c2.label)||boost::algorithm::contains(c2.label, c1.label))
                {
                    c2.docs.clear();
                }
            }
        }
        {
            std::vector<NodeValue> tmp;
            for(std::size_t i=0;i<merged_clusters.size();i++)
            {
                if(!merged_clusters[i].docs.empty()) tmp.push_back(merged_clusters[i]);
            }
            std::swap(merged_clusters, tmp);
        }
        //for(std::size_t i=0;i<merged_clusters.size();i++)
        //{
        //    std::cerr<<"\tAFTER MERGE\t";
        //    NodeDebug_(merged_clusters[i]);
        //}
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
        typedef boost::unordered_map<std::pair<uint32_t, uint32_t>, double> ScoreIndex;
        ScoreIndex score_index;
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
                    double pdiff = small/large;
                    if(pdiff<0.5) {
                        pdiff = 0.0;
                    }
                    else
                    {
                        pdiff = std::sqrt(pdiff);
                    }
                    score *= pdiff;
                }
                matrix[i][j] = score;
                OrderScore os(score, std::make_pair(i, j));
                order_scores.push_back(os);
                score_index[std::make_pair(i, j)] = score;
            }
        }
        std::sort(order_scores.begin(), order_scores.end(), std::greater<OrderScore>());
        typedef boost::unordered_map<uint32_t, uint32_t> Result;
        boost::unordered_map<uint32_t, double> group_min_score;
        Result result;
        uint32_t iresult = 0;
        double max_score = order_scores.front().first;
        double score_threshold = max_score*0.2;
        if(score_threshold<=2.0) score_threshold = 2.0;
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
            //std::cerr<<"\t\t Order : "<<score<<std::endl;
            //std::cerr<<"\t\tT1: "<<title1<<std::endl;
            //std::cerr<<"\t\tT2: "<<title2<<std::endl;
            Result::const_iterator it1 = result.find(i1);
            Result::const_iterator it2 = result.find(i2);
            if(score<score_threshold)
            {
                if(it1==result.end())
                {
                    result.insert(std::make_pair(i1, iresult));
                    iresult++;
                }
                if(it2==result.end())
                {
                    result.insert(std::make_pair(i2, iresult));
                    iresult++;
                }
            }
            else {
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
                else if(it1->second!=it2->second)//to see if they can merge
                {
                    double min1 = 0.0;
                    double min2 = 0.0;
                    uint32_t c1 = 0;
                    uint32_t c2 = 0;
                    for(Result::const_iterator it=result.begin();it!=result.end();it++)
                    {
                        if(it->first==i1)
                        {
                            ++c1;
                            if(min1==0.0 || order_scores[i1].first<min1) min1 = order_scores[i1].first;
                        }
                        if(it->first==i2)
                        {
                            ++c2;
                            if(min2==0.0 || order_scores[i2].first<min2) min2 = order_scores[i2].first;
                        }
                    }
                }
            }
        }
        boost::unordered_set<std::string> changed;
        FinalIndexes final;
        for(Result::const_iterator it=result.begin();it!=result.end();++it)
        {
            final[it->second].push_back(it->first);
        }
        for(FinalIndexes::iterator it=final.begin();it!=final.end();++it)
        {
            std::sort(it->second.begin(), it->second.end());
        }
        //try to post merge clusters
        for(FinalIndexes::iterator it=final.begin();it!=final.end();++it)
        {
            if(it->second.size()<2) continue;
            double inner_min1 = 0.0;
            for(uint32_t i=0;i<it->second.size();i++)
            {
                for(uint32_t j=i+1;j<it->second.size();j++)
                {
                    std::pair<uint32_t, uint32_t> index(it->second[i], it->second[j]);
                    ScoreIndex::const_iterator sit = score_index.find(index);
                    if(sit!=score_index.end())
                    {
                        if(inner_min1==0.0||sit->second<inner_min1)
                        {
                            inner_min1 = sit->second;
                        }
                    }
                }
            }
            FinalIndexes::iterator it2 = it;
            ++it2;
            for(;it2!=final.end();it2++)
            {
                if(it2->second.size()<2) continue;
                double inner_min2 = 0.0;
                for(uint32_t i=0;i<it2->second.size();i++)
                {
                    for(uint32_t j=i+1;j<it2->second.size();j++)
                    {
                        std::pair<uint32_t, uint32_t> index(it2->second[i], it2->second[j]);
                        ScoreIndex::const_iterator sit = score_index.find(index);
                        if(sit!=score_index.end())
                        {
                            if(inner_min2==0.0||sit->second<inner_min2)
                            {
                                inner_min2 = sit->second;
                            }
                        }
                    }
                }
                double connect_min = 0.0;
                for(uint32_t i=0;i<it->second.size();i++)
                {
                    for(uint32_t j=0;j<it2->second.size();j++)
                    {
                        std::pair<uint32_t, uint32_t> index(it->second[i], it2->second[j]);
                        ScoreIndex::const_iterator sit = score_index.find(index);
                        if(sit!=score_index.end())
                        {
                            if(connect_min==0.0||sit->second<connect_min)
                            {
                                connect_min = sit->second;
                            }
                        }
                    }
                }
                if(connect_min>=inner_min1||connect_min>=inner_min2)
                {
                    it->second.insert(it->second.end(), it2->second.begin(), it2->second.end());
                    it2->second.clear();
                    std::sort(it->second.begin(), it->second.end());
                    for(uint32_t i=0;i<it->second.size();i++)
                    {
                        for(uint32_t j=i+1;j<it->second.size();j++)
                        {
                            std::pair<uint32_t, uint32_t> index(it->second[i], it->second[j]);
                            ScoreIndex::const_iterator sit = score_index.find(index);
                            if(sit!=score_index.end())
                            {
                                if(inner_min1==0.0||sit->second<inner_min1)
                                {
                                    inner_min1 = sit->second;
                                }
                            }
                        }
                    }
                }
            }
        }

        //std::cerr<<"Clusters count "<<final.size()<<std::endl;
        for(FinalIndexes::const_iterator it=final.begin();it!=final.end();++it)
        {
            //std::cerr<<"Cluster "<<it->first<<" : "<<std::endl;
            if(it->second.empty()) continue;
            if(it->second.size()>max_cluster_size_) max_cluster_size_ = it->second.size();
            const Document& ref = docs[it->second.front()];
            std::string ref_docid;
            ref.getString("DOCID", ref_docid);
            for(std::size_t i=0;i<it->second.size();i++)
            {
                const Document& doc = docs[it->second[i]];
                std::string docid;
                doc.getString("DOCID", docid);
                mresult[docid] = ref_docid;
                changed.insert(docid);
                //std::string title;
                //doc.getString("Title", title);
                //std::string sprice;
                //doc.getString("Price", sprice);
                //std::cerr<<"\t"<<title<<"\t"<<sprice<<std::endl;
            }
        }
        for(uint32_t i=0;i<docs.size();i++)
        {
            const Document& doc = docs[i];
            std::string docid;
            doc.getString("DOCID", docid);
            if(changed.find(docid)==changed.end())
            {
                mresult.erase(docid);
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
            //if(frag.size()>7||frag.size()<3) continue;
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
        value.score = std::log((double)value.docs.size())*(double)(value.label.size());
    }

private:
    idmlib::util::IDMAnalyzer* analyzer_;
    std::size_t n_;
    Tree tree_;
    std::map<uint32_t, std::string> term2str_;
    std::size_t max_cluster_size_;
};

NS_SF1R_B5M_END

#endif


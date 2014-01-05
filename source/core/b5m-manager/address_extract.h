#ifndef SF1R_B5MMANAGER_ADDRESSEXTRACT_H
#define SF1R_B5MMANAGER_ADDRESSEXTRACT_H 
#include <util/ustring/UString.h>
#include "b5m_types.h"
#include "scd_doc_processor.h"
//#define ADDR_DEBUG
NS_SF1R_B5M_BEGIN

class AddressExtract{
    enum SUFFIX_STATUS {NOT_SUFFIX, SUFFIX_NEXT, SUFFIX_BREAK, SUFFIX_DICT};
    enum ADDR_TYPE {NOT_ADDR = 1, PROVINCE, CITY, COUNTY, DISTRICT, ROAD, POI};
    enum FLAG_TYPE {LEFT_CONTEXT=101, RIGHT_CONTEXT, SUFFIX, ADDR};
    typedef uint32_t term_t;
    typedef std::vector<term_t> word_t;
    typedef std::pair<word_t, ADDR_TYPE> token_t;
    typedef std::map<word_t, ADDR_TYPE> Dict;
    typedef std::map<word_t, double> Prob;
    typedef std::map<word_t, std::size_t> Count;
    typedef std::map<std::string, std::string> SynDict;
    typedef izenelib::util::UString UString;
    struct Node
    {
        std::size_t start;
        std::size_t end;
        term_t type;
        double prob;
        std::string text;
        bool operator<(const Node& n) const
        {
            if(type!=n.type) return type<n.type;
            return start>n.start;
        }
    };
    struct SuffixTerm
    {
        term_t term;
        std::string text;
        ADDR_TYPE type;
        bool is_suffix;
        double prob;
        std::size_t len;
        bool in_dict;

        SuffixTerm() : term(0), type(NOT_ADDR), is_suffix(false), prob(0.0), len(0), in_dict(false)
        {
        }
    };
    typedef std::vector<SuffixTerm> SuffixWord;
    typedef std::vector<Node> NodeList;
    typedef std::vector<NodeList> NodeMatrix;
    //enum {STATUS_NO, STATUS_NEXT, STATUS_YES};
public:
    AddressExtract(): ADDR_PROP_NAME("MerchantAddr")
                      //, NOT_ADDR(1), PROVINCE(2), CITY(3), COUNTY(4), DISTRICT(5), ROAD(6), POI(7)
                      //, LEFT_CONTEXT(101), RIGHT_CONTEXT(102), SUFFIX(103), ADDR(104)
    {
        ADDR_LIST.push_back(PROVINCE);
        ADDR_LIST.push_back(CITY);
        ADDR_LIST.push_back(COUNTY);
        ADDR_LIST.push_back(DISTRICT);
        ADDR_LIST.push_back(ROAD);
        ADDR_LIST.push_back(POI);
        idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        //csconfig.symbol = true;
        analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
    }
    ~AddressExtract()
    {
        delete analyzer_;
    }

    void Process(const std::string& knowledge)
    {
        Train(knowledge);
        std::string scd = knowledge+"/T.SCD";
        ScdDocProcessor processor(boost::bind(&AddressExtract::EvaluateDoc_, this, _1), 1);
        processor.AddInput(scd);
        processor.Process();
    }

    bool Train(const std::string& knowledge)
    {
        std::string dict = knowledge+"/dict";
        std::string syn_dict = knowledge+"/dict.syn";
        std::string scd = knowledge+"/T.SCD";
        std::string line;
        std::ifstream sifs(syn_dict.c_str());
        while( getline(sifs, line) )
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of("@"));
            if(vec.size()!=2) continue;
            std::string low = vec[0];
            std::string high = vec[1];
            if(low==high) continue;
            if(low>high) std::swap(low, high);
            syn_dict_[low] = high;
            syn_dict_[high] = low;
        }
        sifs.close();
        std::ifstream ifs(dict.c_str());
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of("@"));
            if(vec.size()!=2) continue;
            const std::string& str = vec[0];
            word_t word;
            GetWord_(str, word);
            word_t rword(word.rbegin(), word.rend());
            ADDR_TYPE type = NOT_ADDR;
            if(vec[1]=="province")
            {
                type = PROVINCE;
            }
            else if(vec[1]=="city")
            {
                type = CITY;
            }
            else if(vec[1]=="county")
            {
                type = COUNTY;
            }
            else if(vec[1]=="district")
            {
                type = DISTRICT;
            }
            else if(vec[1]=="road")
            {
                type = ROAD;
            }
            else if(vec[1]=="poi")
            {
                type = POI;
            }
            if(type==POI&&word.size()<=2) continue;
            if(type==ROAD&&word.size()<=2) continue;
            dict_[word] = type;
            rdict_[rword] = type;
            //word_text_[word] = line;
            word_t syn_word;
            SynDict::const_iterator sdit = syn_dict_.find(str);
            if(sdit!=syn_dict_.end())
            {
                const std::string& syn = sdit->second;
                GetWord_(syn, syn_word);
            }
            //if(type==PROVINCE)
            //{
            //    if(word.size()>=3&&GetText_(word.back())=="省")
            //    {
            //        syn_word.assign(word.begin(), word.end()-1);
            //    }
            //}
            //else if(type==CITY)
            //{
            //    if(word.size()>=3&&GetText_(word.back())=="市")
            //    {
            //        syn_word.assign(word.begin(), word.end()-1);
            //    }
            //}
            if(!syn_word.empty())
            {
                word_t rsyn_word(syn_word.rbegin(), syn_word.rend());
                dict_[syn_word] = type;
                rdict_[rsyn_word] = type;
            }
        }
        std::cerr<<"dict size "<<dict_.size()<<std::endl;
        std::cerr<<"rdict size "<<rdict_.size()<<std::endl;
        ifs.close();
        ScdDocProcessor processor(boost::bind(&AddressExtract::ProcessTrainDoc_, this, _1), 1);
        processor.AddInput(scd);
        processor.Process();
        for(Count::const_iterator it=count_.begin();it!=count_.end();++it)
        {
            const word_t& key = it->first;
            FLAG_TYPE type = (FLAG_TYPE)key[0];
            if(type==LEFT_CONTEXT || type==RIGHT_CONTEXT)
            {
                term_t addr = key[1];
                word_t addrv(1, addr);
                double p = (double)it->second/count_[addrv];
                prob_[key] = p;
            }
            else if(type==SUFFIX)
            {
                //term_t term = key.back();
                term_t addr = key[1];
                word_t addrv(1, addr);
                double p = (double)it->second/count_[addrv];
                prob_[key] = p;
            }
        }
        return true;
    }

    //void Evaluate(const std::string& address)
    //{
    //    word_t word;
    //    GetWord_(address, word);
    //    word_t rword(word.rbegin(), word.rend());
    //    std::vector<token_t> tokens;
    //    GetTokens_(word, tokens);
    //    for(uint32_t i=0;i<tokens.size();i++)
    //    {
    //        const token_t& token = tokens[i];
    //        if(token.second!=NOT_ADDR) continue;
    //        const word_t& word = token.first;
    //        std::string wtext = GetText_(word);
    //        for(uint32_t w=1;w<word.size();w++)
    //        {
    //            term_t term = word[w];
    //            word_t left;
    //            if(i>0)
    //            {
    //                left.push_back(tokens[i-1].first.back());
    //            }
    //            left.push_back(word[w-1]);
    //            word_t right;
    //            if(w<word.size()-1) right.push_back(word[w+1]);
    //            if(w<word.size()-2) right.push_back(word[w+2]);
    //            word_t textw(word.begin(), word.begin()+w+1);
    //            std::string text = GetText_(textw);
    //            std::string left_text;
    //            if(!left.empty()) left_text = GetText_(left);
    //            std::string right_text;
    //            if(!right.empty()) right_text = GetText_(right);
    //            for(uint32_t l=0;l<ADDR_LIST.size();l++)
    //            {
    //                word_t key(3);
    //                key[0] = SUFFIX;
    //                key[1] = ADDR_LIST[l];
    //                key[2] = term;
    //                Prob::const_iterator it = prob_.find(key);
    //                double prob = 0.0;
    //                if(it!=prob_.end())
    //                {
    //                    prob = it->second;
    //                }
    //                if(prob<0.2) continue;
    //                double ru_prob = 0.0;
    //                double rb_prob = 0.0;
    //                if(right.size()>=1)
    //                {
    //                    key.resize(3);
    //                    key[0] = RIGHT_CONTEXT;
    //                    key[1] = ADDR_LIST[l];
    //                    key[2] = right[0];
    //                    it = prob_.find(key);
    //                    if(it!=prob_.end())
    //                    {
    //                        ru_prob = it->second;
    //                    }
    //                }
    //                if(right.size()>=2)
    //                {
    //                    key.resize(4);
    //                    key[0] = RIGHT_CONTEXT;
    //                    key[1] = ADDR_LIST[l];
    //                    key[2] = right[0];
    //                    key[3] = right[1];
    //                    it = prob_.find(key);
    //                    if(it!=prob_.end())
    //                    {
    //                        rb_prob = it->second;
    //                    }
    //                }
    //                double lu_prob = 0.0;
    //                double lb_prob = 0.0;
    //                if(left.size()>=1)
    //                {
    //                    key.resize(3);
    //                    key[0] = LEFT_CONTEXT;
    //                    key[1] = ADDR_LIST[l];
    //                    key[2] = left[0];
    //                    it = prob_.find(key);
    //                    if(it!=prob_.end())
    //                    {
    //                        lu_prob = it->second;
    //                    }
    //                }
    //                if(left.size()>=2)
    //                {
    //                    key.resize(4);
    //                    key[0] = LEFT_CONTEXT;
    //                    key[1] = ADDR_LIST[l];
    //                    key[2] = left[0];
    //                    key[3] = left[1];
    //                    it = prob_.find(key);
    //                    if(it!=prob_.end())
    //                    {
    //                        lb_prob = it->second;
    //                    }
    //                }
    //                std::cerr<<"find candidate "<<text<<","<<left_text<<","<<right_text<<" in "<<ADDR_LIST[l]<<" : "<<prob<<","<<lu_prob<<","<<lb_prob<<","<<ru_prob<<","<<rb_prob<<std::endl;
    //                std::cerr<<"\tall text "<<wtext<<std::endl;
    //                if(i>0)
    //                {
    //                    std::cerr<<"\tleft addr "<<GetText_(tokens[i-1].first)<<","<<tokens[i-1].second<<std::endl;
    //                }
    //            }
    //        }
    //    }
    //}
    std::string Evaluate(const std::string& address)
    {
        if(address.length()>300) return "";
        //word_t word;
        //GetWord_(address, word);
        //word_t rword(word.rbegin(), word.rend());
        SuffixWord word;
        GetSuffixWord_(address, word);
#ifdef ADDR_DEBUG
        std::cerr<<"[A]"<<address<<",["<<word.size()<<"]"<<std::endl;
#endif
        NodeList current;
        NodeMatrix global;
        Recursive_(word, 0, 0, current, global);
        for(uint32_t i=0;i<global.size();i++)
        {
            NodeList& nl = global[i];
            PostProcessResult_(nl, word);
        }
        std::pair<double, std::size_t> select(0.0, 0);
        for(uint32_t i=0;i<global.size();i++)
        {
            double score = 0.0;
            const NodeList& nl = global[i];
            for(uint32_t j=0;j<nl.size();j++)
            {
                const Node& node = nl[j];
                score+=node.prob;
            }
            std::string str = GetText_(nl);
#ifdef ADDR_DEBUG
            std::cerr<<"[RESULT-SCORE]"<<str<<","<<score<<std::endl;
#endif
            if(score>select.first)
            {
                select.first = score;
                select.second = i;
            }
        }
        std::string result;
        if(select.first>0.0)
        {
            const NodeList& nresult = global[select.second];
            result = GetText_(nresult);
        }
#ifdef ADDR_DEBUG
        std::cerr<<"[FINAL]"<<result<<std::endl;
#endif
        return result;
    }


private:
    void PostProcessResult_(NodeList& nl, const SuffixWord& word)
    {
        for(uint32_t i=0;i<nl.size();i++)
        {
            Node& node = nl[i];
            node.text = GetText_(word, node.start, node.end);
            SynDict::const_iterator it = syn_dict_.find(node.text);
            if(it!=syn_dict_.end())
            {
                if(it->second>node.text)
                {
                    node.text = it->second;
                }
            }
        }
        std::sort(nl.begin(), nl.end());
        bool changed = false;
        for(uint32_t i=0;i<nl.size();i++)
        {
            Node& ni = nl[i];
            if(ni.text.empty()) continue;
            for(uint32_t j=i+1;j<nl.size();j++)
            {
                Node& nj = nl[j];
                if(nj.text.empty()) continue;
                if(ni.text==nj.text)
                {
                    changed = true;
                    if(nj.prob>ni.prob) ni.text.clear();
                    else nj.text.clear();
                }
            }
        }
        if(changed)
        {
            NodeList newlist;
            for(uint32_t i=0;i<nl.size();i++)
            {
                if(!nl[i].text.empty()) newlist.push_back(nl[i]);
            }
            std::swap(nl, newlist);
        }

    }
    void GetSuffixWord_(const std::string& text, SuffixWord& word)
    {
        typedef std::vector<idmlib::util::IDMTerm> TermList;
        TermList term_list;
        UString utext(text, UString::UTF_8);
        analyzer_->GetTermList(utext, term_list);
        for(TermList::const_reverse_iterator it = term_list.rbegin(); it!=term_list.rend(); ++it)
        {
            SuffixTerm st;
            st.text = it->TextString();
            st.term = izenelib::util::HashFunction<std::string>::generateHash32(st.text);
            word.push_back(st);
        }
        for(uint32_t i=0;i<word.size();i++)
        {
            SuffixTerm& st = word[i];
            std::size_t select_j = 0;
            double max_weight = 0.0;
            ADDR_TYPE max_type=NOT_ADDR;
            SUFFIX_STATUS max_status = NOT_SUFFIX;
            std::size_t last_dict_j = 0;
            for(std::size_t j=i+2;j<=word.size();j++)
            {
                word_t frag(j-i);
                for(std::size_t f=i;f<j;f++)
                {
                    frag[f-i] = word[f].term;
                }
                word_t left;
                if(j<word.size()) left.push_back(word[j].term);
                if(j<word.size()-1) 
                {
                    left.push_back(word[j+1].term);
                    //std::swap(left[0], left[1]);
                }
                double weight = 0.0;
                ADDR_TYPE type = NOT_ADDR;
                SUFFIX_STATUS status = NOT_SUFFIX;
                if(last_dict_j>0 && j-last_dict_j>2) status = NOT_SUFFIX;
                else
                {
                    status = GetSuffixStatus_(frag, left, type, weight);
                }
                int force_select = 0;
                if(last_dict_j>0)
                {
                    if(status==SUFFIX_NEXT) force_select = -1;
                    else if(status==SUFFIX_DICT)
                    {
                        force_select = 1;
                    }
                }
#ifdef ADDR_DEBUG
                std::string frag_str = GetText_(word, i, j);
                std::cerr<<"[SUFFIX_STATUS]"<<frag_str<<","<<status<<","<<type<<","<<weight<<std::endl;
#endif
                if(status==SUFFIX_DICT||status==SUFFIX_NEXT||status==SUFFIX_BREAK)
                {
                    if(force_select>=0 && (weight>max_weight || force_select>0))
                    {
                        max_weight = weight;
                        max_type = type;
                        select_j = j;
                        max_status = status;
                    }
                }
                if(status==NOT_SUFFIX || status==SUFFIX_BREAK) break;
                if(status==SUFFIX_DICT) last_dict_j = j;
            }
            if(select_j==0) //not found
            {
            }
            else
            {
                st.type = max_type;
                st.is_suffix = true;
                st.prob = max_weight;
                st.len = select_j-i;
                if(max_status==SUFFIX_DICT)
                {
                    st.in_dict = true;
                }
#ifdef ADDR_DEBUG
                std::cerr<<"[FIND]"<<GetText_(word, i, select_j)<<std::endl;
#endif
            }
        }
    }
    void Recursive_(const SuffixWord& word, std::size_t start, std::size_t maxi_limit, NodeList& current, NodeMatrix& global)
    {
#ifdef ADDR_DEBUG
        std::string sw = GetText_(word, start, word.size());
        std::cerr<<"[D1]"<<sw<<","<<start<<","<<maxi_limit<<std::endl;
#endif
        std::size_t i=start;
        while(i<word.size())
        {
            const SuffixTerm& st = word[i];
#ifdef ADDR_DEBUG
            std::cerr<<"[SUFFIXTERM]"<<i<<","<<st.is_suffix<<","<<st.len<<","<<st.type<<","<<st.prob<<std::endl;
#endif
            if(st.is_suffix&&st.len>0)
            {
#ifdef ADDR_DEBUG
                std::cerr<<"[FIND]"<<GetText_(word, i, i+st.len)<<std::endl;
#endif
                NodeList bc(current);
                Node node;
                node.start = i;
                node.end = i+st.len;
                node.type = st.type;
                node.prob = st.prob;
                current.push_back(node);
                Recursive_(word, i+st.len, 0, current, global);
                if(st.len>2)
                {
                    Recursive_(word, i+1, i+st.len, bc, global);
                }
                break;
            }
            else
            {
                ++i;
                if(maxi_limit>0&&i>=maxi_limit) break;
                if(i>=word.size()) break;
            }
        }
        if(i>=word.size())//finish search
        {
            global.push_back(current);
        }
    }
//    void Recursive_(const word_t& word, std::size_t start, std::size_t maxi_limit, NodeList& current, NodeMatrix& global)
//    {
//        word_t cword(word.begin()+start, word.end());
//        word_t fword(cword.rbegin(), cword.rend());
//        std::string sw = GetText_(fword);
//#ifdef ADDR_DEBUG
//        std::cerr<<"[D1]"<<sw<<","<<start<<","<<maxi_limit<<std::endl;
//#endif
//        std::size_t i=start;
//        while(true)
//        {
//            //term_t term = word[i];
//            term_t type=NOT_ADDR;
//            std::size_t select_j = 0;
//            double max_weight = 0.0;
//            term_t max_type=NOT_ADDR;
//            for(std::size_t j=i+2;j<=word.size();j++)
//            {
//                word_t frag(word.begin()+i, word.begin()+j);
//                word_t left;
//                if(j<word.size()) left.push_back(word[j]);
//                if(j<word.size()-1) 
//                {
//                    left.push_back(word[j+1]);
//                    std::swap(left[0], left[1]);
//                }
//                double weight = 0.0;
//                int status = IsCandidate_(frag, left, type, weight);
//#ifdef ADDR_DEBUG
//                word_t ffrag(frag.rbegin(), frag.rend());
//                std::cerr<<"[F]"<<GetText_(ffrag)<<","<<status<<std::endl;
//#endif
//                if(status==STATUS_YES||status==STATUS_NEXT)
//                {
//                    if(weight>max_weight)
//                    {
//                        max_weight = weight;
//                        max_type = type;
//                        select_j = j;
//                    }
//                }
//                if(status==STATUS_YES || status==STATUS_NO) break;
//            }
//#ifdef ADDR_DEBUG
//            std::cerr<<"[SJ]"<<i<<","<<select_j<<std::endl;
//#endif
//            if(select_j>0)
//            {
//#ifdef ADDR_DEBUG
//                word_t find_word(word.begin()+i, word.begin()+select_j);
//                word_t ffind_word(find_word.rbegin(), find_word.rend());
//                std::cerr<<"[FIND]"<<GetText_(ffind_word)<<std::endl;
//#endif
//                NodeList bc(current);
//                Node node;
//                node.start = i;
//                node.end = select_j;
//                node.type = type;
//                current.push_back(node);
//                Recursive_(word, select_j, 0, current, global);
//                if(select_j-i>2)
//                {
//                    Recursive_(word, i+1, select_j, bc, global);
//                }
//                break;
//            }
//            else
//            {
//                ++i;
//                if(maxi_limit>0&&i>=maxi_limit) break;
//                if(i>=word.size()) break;
//            }
//        }
//        if(i>=word.size())//finish search
//        {
//            global.push_back(current);
//        }
//    }
    bool IsValidSpecifySuffix_(term_t term, term_t type, double& prob)
    {
        word_t key(3);
        key[0] = SUFFIX;
        key[1] = type;
        key[2] = term;
        Prob::const_iterator it = prob_.find(key);
        if(it!=prob_.end())
        {
            prob = it->second;
        }
        if(prob<0.2) return false;
        return true;
    }
    bool IsValidSuffix_(term_t term, term_t& type)
    {
        std::pair<double, term_t> max_type(0.0, NOT_ADDR);
        for(uint32_t l=0;l<ADDR_LIST.size();l++)
        {
            term_t type = ADDR_LIST[l];
            double prob = 0.0;
            if(!IsValidSpecifySuffix_(term, type, prob)) continue;
            else
            {
                if(prob>max_type.first)
                {
                    max_type.first = prob;
                    max_type.second = type;
                }
            }
        }
        if(max_type.first>0.0)
        {
            type = max_type.second;
            return true;
        }
        return false;
    }
    ADDR_TYPE GetSuffixType_(const word_t& word, std::size_t d, double& prob)
    {
        if(word.size()<=d) return NOT_ADDR;
        std::pair<double, ADDR_TYPE> select(0.0, NOT_ADDR);
        for(std::size_t i=0;i<ADDR_LIST.size();i++)
        {
            ADDR_TYPE type = ADDR_LIST[i];
            double uprob = -1.0;
            double bprob = -1.0;
            word_t key(1, SUFFIX);
            key.push_back(type);
            key.push_back(word[d]);
            Prob::const_iterator it = prob_.find(key);
            if(it!=prob_.end()) uprob = it->second;
            else uprob = 0.0;
#ifdef ADDR_DEBUG
            //std::cerr<<"[SP-U]"<<(int)type<<","<<GetText_(word[d])<<","<<uprob<<std::endl;
#endif
            if(word.size()>d+1) 
            {
                key.push_back(word[d+1]);
                Prob::const_iterator it = prob_.find(key);
                if(it!=prob_.end()) bprob = it->second;
                else bprob = 0.0;
                bprob *= 2;
#ifdef ADDR_DEBUG
                //std::cerr<<"[SP-B]"<<(int)type<<","<<GetText_(word[d+1])<<","<<bprob<<std::endl;
#endif
            }
            double cprob = bprob>uprob ? bprob : uprob;
            if(cprob>select.first)
            {
                select.first = cprob;
                select.second = type;
            }
        }
        if(select.first>=0.1)
        {
            prob = select.first;
            return select.second;
        }
        return NOT_ADDR;
    }
    SUFFIX_STATUS GetSuffixStatus_(const word_t& tword, const word_t& left, ADDR_TYPE& type, double& weight)
    {
        Dict::const_iterator it = rdict_.lower_bound(tword);
        if(it!=rdict_.end())
        {
            if(boost::algorithm::starts_with(it->first, tword))
            {
                if(it->first==tword)
                {
                    type = it->second;
                    weight = 1.0;
                    return SUFFIX_DICT;
                }
                else 
                {
                    type = it->second;
                    weight = 0.0;
                    return SUFFIX_NEXT;
                }
            }
        }
        //static const std::size_t SUFFIX_SEARCH_SPACE = 2;
        double suffix_prob = 0.0;
        ADDR_TYPE suffix_type = GetSuffixType_(tword, 0, suffix_prob);
        if(suffix_type==NOT_ADDR||suffix_type==PROVINCE||suffix_type==CITY)
        {
            return NOT_SUFFIX;
        }
        type = suffix_type;
        if(left.empty())
        {
            //if(type==PROVINCE || type==CITY || type==COUNTY || type==DISTRICT) return STATUS_YES;
            weight = 0.8;
            return SUFFIX_BREAK;
        }
        double left_suffix_prob = 0.0;
        ADDR_TYPE left_suffix_type = GetSuffixType_(left, 0, left_suffix_prob);
        if(left_suffix_type!=NOT_ADDR) 
        {
            weight = 0.6;
            return SUFFIX_BREAK;
        }
        //term_t first = word.front();
        //term_t first_type = NOT_ADDR;
        //if(!IsValidSuffix_(first, first_type))
        //{
        //    return STATUS_NO;
        //}
        double lu_prob = 0.0;
        double lb_prob = 0.0;
        word_t key;
        if(left.size()>=1)
        {
            key.resize(3);
            key[0] = LEFT_CONTEXT;
            key[1] = type;
            key[2] = left[0];
            Prob::const_iterator it = prob_.find(key);
            if(it!=prob_.end())
            {
                lu_prob = it->second;
            }
        }
        if(left.size()>=2)
        {
            key.resize(4);
            key[0] = LEFT_CONTEXT;
            key[1] = type;
            key[2] = left[0];
            key[3] = left[1];
            Prob::const_iterator it = prob_.find(key);
            if(it!=prob_.end())
            {
                lb_prob = it->second;
            }
        }
        double lc_prob = lb_prob>0.0 ? lb_prob : lu_prob;//left context prob
        //double last_suffix_prob = 0.0;
        //bool last_maybe_suffix = false;
        //term_t last = word.back();
        //if(IsValidSpecifySuffix_(last, type, last_suffix_prob))
        //{
        //    last_maybe_suffix = true;
        //}
#ifdef ADDR_DEBUG
        //std::cerr<<"[LPROB]"<<lc_prob<<std::endl;
#endif
        double left_prob = left_suffix_prob;
        if(lc_prob>0.0) left_prob*=2;
        //if(word.size()>=4 && l_prob<0.01 && last_maybe_suffix)
        //{
        //    return STATUS_NO;
        //}
        weight = left_prob*tword.size();
        return SUFFIX_NEXT;
    }
//    int IsCandidate_(const word_t& word, const word_t& left, term_t& type, double& weight)
//    {
//        Dict::const_iterator it = rdict_.lower_bound(word);
//        if(it!=rdict_.end())
//        {
//            if(boost::algorithm::starts_with(it->first, word))
//            {
//                if(it->first==word)
//                {
//                    type = it->second;
//                    weight = 1.0;
//                    return STATUS_YES;
//                }
//                else 
//                {
//                    type = it->second;
//                    weight = 0.0;
//                    return STATUS_NEXT;
//                }
//            }
//        }
//        //static const std::size_t SUFFIX_SEARCH_SPACE = 2;
//        double suffix_prob = 0.0;
//        term_t suffix_type = GetSuffixType_(word, 0, suffix_prob);
//        if(suffix_type==NOT_ADDR||suffix_type==PROVINCE||suffix_type==CITY)
//        {
//#ifdef ADDR_DEBUG
//            std::cerr<<"[ST]"<<suffix_type<<std::endl;
//#endif
//            return STATUS_NO;
//        }
//        type = suffix_type;
//        if(left.empty())
//        {
//            //if(type==PROVINCE || type==CITY || type==COUNTY || type==DISTRICT) return STATUS_YES;
//            weight = 0.8;
//            return STATUS_YES;
//        }
//        word_t rleft(left.rbegin(), left.rend());
//        double left_suffix_prob = 0.0;
//        term_t left_suffix_type = GetSuffixType_(rleft, 0, left_suffix_prob);
//        if(left_suffix_type!=NOT_ADDR) 
//        {
//            weight = 0.6;
//            return STATUS_YES;
//        }
//        //term_t first = word.front();
//        //term_t first_type = NOT_ADDR;
//        //if(!IsValidSuffix_(first, first_type))
//        //{
//        //    return STATUS_NO;
//        //}
//        double lu_prob = 0.0;
//        double lb_prob = 0.0;
//        word_t key;
//        if(left.size()>=1)
//        {
//            key.resize(3);
//            key[0] = LEFT_CONTEXT;
//            key[1] = type;
//            key[2] = left[0];
//            Prob::const_iterator it = prob_.find(key);
//            if(it!=prob_.end())
//            {
//                lu_prob = it->second;
//            }
//        }
//        if(left.size()>=2)
//        {
//            key.resize(4);
//            key[0] = LEFT_CONTEXT;
//            key[1] = type;
//            key[2] = left[0];
//            key[3] = left[1];
//            Prob::const_iterator it = prob_.find(key);
//            if(it!=prob_.end())
//            {
//                lb_prob = it->second;
//            }
//        }
//        double lc_prob = lb_prob>0.0 ? lb_prob : lu_prob;//left context prob
//        //double last_suffix_prob = 0.0;
//        //bool last_maybe_suffix = false;
//        //term_t last = word.back();
//        //if(IsValidSpecifySuffix_(last, type, last_suffix_prob))
//        //{
//        //    last_maybe_suffix = true;
//        //}
//#ifdef ADDR_DEBUG
//        std::cerr<<"[LPROB]"<<lc_prob<<std::endl;
//#endif
//        double left_prob = left_suffix_prob;
//        if(lc_prob>0.0) left_prob*=2;
//        //if(word.size()>=4 && l_prob<0.01 && last_maybe_suffix)
//        //{
//        //    return STATUS_NO;
//        //}
//        weight = left_prob*word.size();
//        return STATUS_NEXT;
//    }

    void ProcessTrainDoc_(ScdDocument& doc)
    {
        std::string address;
        doc.getString(ADDR_PROP_NAME, address);
        if(address.empty()) return;
        word_t word;
        GetWord_(address, word);
        std::vector<token_t> tokens;
        //std::cerr<<"After gettoken "<<address<<","<<word.size()<<std::endl;
        GetTokens_(word, tokens);
        for(uint32_t i=0;i<tokens.size();i++)
        {
            const token_t& current = tokens[i];
            if(current.second==NOT_ADDR) continue;
            const word_t& current_word = current.first;
            word_t key(1, SUFFIX);
            key.push_back(current.second);
            key.push_back(current_word.back());
            AddCount_(key, 1);
            if(current_word.size()>1)
            {
                key.push_back(current_word[current_word.size()-2]);
                AddCount_(key, 1);
            }
            key.clear();
            key.push_back(ADDR);
            AddCount_(key, 1);
            key.clear();
            key.push_back(current.second);
            AddCount_(key, 1);
            if(i>0)
            {
                const token_t& left = tokens[i-1];
                const word_t& left_word = left.first;
                word_t key(1, LEFT_CONTEXT);
                key.push_back(left.second);
                key.push_back(left_word.back());
                AddCount_(key, 1);
                if(left_word.size()>=2)
                {
                    word_t key(1, LEFT_CONTEXT);
                    key.push_back(left.second);
                    key.push_back(*(left_word.end()-1));
                    key.push_back(*(left_word.end()-2));
                    AddCount_(key, 1);
                }
            }
            if(i<tokens.size()-1)
            {
                const token_t& right = tokens[i+1];
                const word_t& right_word = right.first;
                word_t key(1, RIGHT_CONTEXT);
                key.push_back(right.second);
                key.push_back(right_word.back());
                AddCount_(key, 1);
                if(right_word.size()>=2)
                {
                    word_t key(1, RIGHT_CONTEXT);
                    key.push_back(right.second);
                    key.push_back(*(right_word.begin()+1));
                    key.push_back(*(right_word.begin()));
                    AddCount_(key, 1);
                }
            }

        }
    }
    void EvaluateDoc_(ScdDocument& doc)
    {
        std::string address;
        doc.getString(ADDR_PROP_NAME, address);
        if(address.empty()) return;
        //std::cerr<<"[A]"<<address<<std::endl;
        std::string r = Evaluate(address);
        //std::cerr<<"[R]"<<r<<std::endl;
    }
    void GetWord_(const std::string& text, word_t& word)
    {
        std::vector<idmlib::util::IDMTerm> term_list;
        UString utext(text, UString::UTF_8);
        analyzer_->GetTermList(utext, term_list);
        for(uint32_t i=0;i<term_list.size();i++)
        {
            std::string str = term_list[i].TextString();
            word.push_back(GetTerm_(str));
        }
    }
    term_t GetTerm_(const std::string& str)
    {
        term_t term = izenelib::util::HashFunction<std::string>::generateHash32(str);
        //term_text_[term] = str;
        return term;
    }
    //std::string GetText_(term_t term)
    //{
    //    return term_text_[term];
    //}
    //std::string GetText_(const word_t& word)
    //{
    //    std::string r;
    //    for(uint32_t i=0;i<word.size();i++)
    //    {
    //        std::string s = term_text_[word[i]];
    //        if(s.empty()) s = " ";
    //        r+=s;
    //    }
    //    return r;
    //}
    std::string GetText_(const SuffixWord& word, std::size_t start, std::size_t end)
    {
        std::string str;
        for(SuffixWord::const_iterator it = word.begin()+start; it!=word.begin()+end; ++it)
        {
            str=it->text+str;
        }
        return str;
    }
    std::string GetText_(const NodeList& nl)
    {
        std::string result;
        for(std::size_t i=0;i<nl.size();i++)
        {
            const Node& node = nl[i];
            if(!result.empty()) result+="|";
            result+=node.text;
        }
        return result;
    }

    void GetTokens_(const word_t& word, std::vector<token_t>& tokens)
    {
        uint32_t start=0;
        std::string noaddr_token;
        word_t notaddr;
        while(true)
        {
            if(start>=word.size()) break;
            uint32_t len=1;
            word_t find_word;
            ADDR_TYPE find_type=NOT_ADDR;
            while(true)
            {
                uint32_t end=start+len;
                if(end>word.size()) break;
                word_t frag(word.begin()+start, word.begin()+end);
                Dict::const_iterator it = dict_.lower_bound(frag);
                if(it==dict_.end()) break;
                if(!boost::algorithm::starts_with(it->first, frag)) break;
                if(it->first==frag)
                {
                    find_word = frag;
                    find_type = it->second;
                }
                ++len;
            }
            if(!find_word.empty())
            {
                start += find_word.size();
                if(!notaddr.empty())
                {
                    tokens.push_back(std::make_pair(notaddr, NOT_ADDR));
                    notaddr.clear();
                }
                tokens.push_back(std::make_pair(find_word, find_type));
            }
            else
            {
                notaddr.push_back(word[start]);
                ++start;
                //if(start<word.size())
                //{
                //    std::string str = word.substr(start, 1);
                //    noaddr_token += str;
                //}
            }
        }
        if(!notaddr.empty())
        {
            tokens.push_back(std::make_pair(notaddr, NOT_ADDR));
        }
    }

    void AddCount_(const word_t& word, std::size_t count)
    {
        Count::iterator it = count_.find(word);
        if(it==count_.end())
        {
            count_.insert(std::make_pair(word, count));
        }
        else
        {
            it->second += count;
        }
    }

private:
    idmlib::util::IDMAnalyzer* analyzer_;
    Dict dict_;
    Dict rdict_;
    std::map<word_t, std::string> word_text_;
    std::map<term_t, std::string> term_text_;
    SynDict syn_dict_;
    Prob prob_;
    Count count_;
    const std::string ADDR_PROP_NAME;
    std::vector<ADDR_TYPE> ADDR_LIST;
};

NS_SF1R_B5M_END

#endif




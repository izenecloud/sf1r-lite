/*
* Copyright 2010, Graham Neubig
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


// A package for Pitman-Yor language models
//  by Graham Neubig
//
// References:
//  Yee Whye Teh
//  "A Bayesian Interpretation of Interpolated Kneser-Ney"
//  NUS School of Computing Technical Report TRA2/06
//
//  gamma sampling:
//  Luc Devroye
//  "Non-Uniform Random Variate Generation"
//  http://cg.scs.carleton.ca/~luc/rnbookindex.html


#ifndef PYLM_H
#define PYLM_H

#include <vector>
#include <tr1/unordered_map>
#include <map>
#include <stdexcept>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#define PRIOR_DA 1.5
#define PRIOR_DB 1.5
#define PRIOR_SA 2.0
#define PRIOR_SB 1.0
#define DEFAULT_DISC 0.5
#define DEFAULT_STREN 1.0

using namespace std::tr1;
using namespace std;
template<class T>
ofstream& operator |(ofstream& ofs,const T& num)
{
        ofs.write((const char*)&num, sizeof(num));
        return ofs;
}
template<class T>
ifstream& operator |(ifstream& ifs,T& num)
{
        ifs.read((char*)&num, sizeof(num));
        return ifs;
}
ofstream& saveString(ofstream& ofs,const string& str)
{
        size_t len=sizeof(str[0])*(str.size());
        ofs.write((const char*)&len, sizeof(len));
        ofs.write((const char*)&str[0], sizeof(str[0])*(str.size()) );
        return ofs;
}
template<class T>
ofstream& saveVector(ofstream& ofs,const vector<T>& vec)
{
        size_t size=vec.size();
        //cout<<"saveVector"<<size<<endl;
        ofs|size;
        if(size>0)
            ofs.write((const char*)&vec[0], sizeof(vec[0])*vec.size());
        return ofs;
}
ifstream& loadString(ifstream& ifs,string& str)
{
        size_t len;
        ifs.read(( char*)&len, sizeof(len));
        str.resize(len);
        ifs.read(( char*)&str[0], len);
        return ifs;
}
template<class T>
ifstream& loadVector(ifstream& ifs,vector<T>& vec)
{
        size_t size=vec.size();
        ifs|size;
        //cout<<"loadVector"<<size<<endl;
        vec.resize(size);
        if(size>0)
            ifs.read(( char*)&vec[0], sizeof(vec[0])*vec.size());
        return ifs;
}

namespace pylm {

typedef double LMProb;
typedef int PyId;
typedef unordered_map<int, int> CountMap;
    
template <class T>  //T: 全局ID WordID
class PyNode {

public:

    typedef map< T, vector<int> > TableMap;//
    typedef unordered_map< T, PyId > NodeMap;
    typedef unordered_map< T, double > EmitProbeMap;
//protected:

    vector< PyNode* > & nodes_;
    PyId pos_;      //NodeID

    T id_;           //WordID
    EmitProbeMap* emitprob_;
    //EmitProbeMap* baseprob_;
    TableMap tables_; //TableMap<first=单词ID,second[0]=∑second[i],i>=1   second[i]坐在第i张桌子的上单词数量>
    NodeMap children_;// WordID->NodeID  nodes_[iter->second].id_=iter->first
    PyId parent_;    //parent NodeID

    int tableCount_, custCount_;

public:
    void show()
    {
         cout<<"pos"<<pos_<<"id"<<id_<<"parent"<<parent_<<"tableCount"<<tableCount_<<"custCount"<<custCount_<<"children_.size():"<<children_.size()<<"tables_.size():"<<tables_.size()<<endl;
         for(typename TableMap::iterator iter=tables_.begin();iter!=tables_.end();iter++)
         {
              cout<<(iter->first)<<"  ";
         }
         cout<<endl;
         for(typename NodeMap::iterator iter=children_.begin();iter!=children_.end();iter++)
         {
              cout<<(iter->first)<<" "<<(iter->second)<<"     ";
         }
         cout<<endl;
    }
    void save(ofstream& ofs)
    {
         ofs|pos_|id_|parent_|tableCount_|custCount_;
         size_t chsize=children_.size(),tbsize=tables_.size();
         ofs|chsize|tbsize;
         for(typename TableMap::iterator iter=tables_.begin();iter!=tables_.end();iter++)
         {
              ofs|(iter->first);
              saveVector(ofs,iter->second);
         }
         for(typename NodeMap::iterator iter=children_.begin();iter!=children_.end();iter++)
         {
              ofs|(iter->first)|(iter->second);
         }
    }

    void load(ifstream& ifs)
    {
         ifs|pos_|id_|parent_|tableCount_|custCount_;
         size_t chsize,tbsize;
         ifs|chsize|tbsize;
         for(size_t i=0;i<tbsize;i++)
         {
             T idt;
             vector<int>  vec;
             ifs|idt;
             loadVector(ifs,vec);
             tables_.insert(pair< T,vector<int> >(idt,vec));
         }
         for(size_t i=0;i<chsize;i++)
         {
             T idt;
             PyId pyid; 
             ifs|idt|pyid;
             children_.insert(pair< T,PyId >(idt,pyid));
         }
    }

    PyNode(vector< PyNode* > & nodes, PyId pos = 0, T id = -1, PyId parent = -1) 
        : nodes_(nodes), pos_(pos), id_(id), tables_(), children_(), parent_(parent), tableCount_(0), custCount_(0)  { emitprob_=new EmitProbeMap();}

    ~PyNode() { }

    void accumulateCounts(vector<unsigned> & counts, int lev) {    //每一级词语总数统计     
        for(typename NodeMap::const_iterator it = children_.begin(); it != children_.end(); it++)
            nodes_[it->second]->accumulateCounts(counts,lev+1);
        counts[lev] += tables_.size();
    }

    void compute(int lev, int max, const string* strs, const LMProb& base, 
                const vector<LMProb> & strens, const vector<LMProb> & discs )  {
        if(lev != max) {
            for(typename NodeMap::const_iterator it = children_.begin(); it != children_.end(); it++)
                nodes_[it->second]->compute(lev+1, max, strs, base, strens, discs);
            return;
        }
        for(typename TableMap::const_iterator it = tables_.begin(); it != tables_.end(); it++) {
            // get the output prob
            LMProb myProb = getEmitProb(it->first, base, strens, discs);
            (*emitprob_)[it->first]=myProb;
        }
    }
    void print(int lev, int max, const string* strs, const LMProb* bases, 
                const vector<LMProb> & strens, const vector<LMProb> & discs, 
                ostream & os = cout) const {
        if(lev != max) {
            for(typename NodeMap::const_iterator it = children_.begin(); it != children_.end(); it++)
                nodes_[it->second]->print(lev+1, max, strs, bases, strens, discs, os);
            return;
        }
        ostringstream buff;
        LMProb log10 = log(10);
        vector<PyId> path;
        if(parent_ != -1) {
            buff << strs[id_].substr(1);
            path.push_back(id_);
            for(PyId node = parent_; node > 0; node = nodes_[node]->parent_) {
                PyId nextId = nodes_[node]->id_;
                buff << " " << strs[nextId].substr(1);
                path.push_back(nextId);//节点路径
            }
        }//缓存word
        string myId = buff.str();
        for(typename TableMap::const_iterator it = tables_.begin(); it != tables_.end(); it++) {
            // get the output prob
            LMProb myProb = getEmitProb(it->first, bases[it->first], strens, discs);
            // find the output node and fall back
            PyId ch = nodes_[0]->findChild(it->first);
            for(int i = path.size()-1; ch != -1 && i >= 0; i--)
                ch = nodes_[ch]->findChild(path[i]);
            os << log(myProb)/log10 << "\t";//输出概率
            os << strs[it->first].substr(1);//
            if(myId.length()) os<<"|" << myId << " ";//word or unk
            if(ch != -1) os << "\t" << log(nodes_[ch]->getFallbackProb(strens[lev],discs[lev]))/log10;//child
            os << endl;
        }
    }

    void trim(const vector<PyId> & nodeIds, const vector<T> & wordIds) {
        pos_ = nodeIds[pos_];
        parent_ = parent_==-1?-1:nodeIds[parent_];
        id_ = (id_==-1?-1:wordIds[id_]);
        TableMap newTabMap;
        for(typename TableMap::const_iterator it = tables_.begin(); it != tables_.end(); it++)
            newTabMap.insert(pair< T, vector<int> >(wordIds[it->first], it->second));
        tables_ = newTabMap;
        NodeMap newChildMap;
        for(typename NodeMap::const_iterator it = children_.begin(); it != children_.end(); it++)
            newChildMap.insert(pair< T, PyId >(wordIds[it->first], nodeIds[it->second]));
        children_ = newChildMap;
    }

    LMProb getFallbackProb(LMProb s, LMProb d) const {
        return (s+tableCount_*d)/(s+custCount_);
    }
    LMProb getLocalProb(T emit, LMProb s, LMProb d) const {//pitman-yor process 概率的一部分
        typename TableMap::const_iterator it = tables_.find(emit);
        if(it == tables_.end()) return 0;
        const vector<int> & tabs = it->second;
        return (tabs[0]-(tabs.size()-1)*d)/(s+custCount_);//s,d pitman-yor process parameter
    }

    int getLevel() const {
        return parent_ == -1 ? 0 : nodes_[parent_]->getLevel()+1;
    }

    LMProb getEmitProb(T emit, LMProb base, const vector<LMProb>& strens, const vector<LMProb>& discs, int lev = -1) const {
       
        if((*emitprob_).find(emit)!=(*emitprob_).end())
            return (*emitprob_)[emit];
        //cout<<"computing"<<endl;
        if(lev == -1)
            lev = getLevel();
        if(parent_ != -1)
            base = nodes_[parent_]->getEmitProb(emit,base,strens,discs,lev-1);// 前一级作 base(x)
        base *= getFallbackProb(strens[lev],discs[lev]);//s,d pitman-yor process parameter 平滑概率
        //cout<<"exactprobe"<<(base+getLocalProb(emit, strens[lev], discs[lev]))<<endl;
        /*
        if((*emitprob_).find(emit)!=(*emitprob_).end())
               cout<<"mapprob"<<(*emitprob_)[emit]<<"exactprob"<<(base+getLocalProb(emit, strens[lev], discs[lev]))<<endl; 
        */
        if((*emitprob_).find(emit)!=(*emitprob_).end())
              (*emitprob_)[emit]=base+getLocalProb(emit, strens[lev], discs[lev]);
        return base+getLocalProb(emit, strens[lev], discs[lev]); //(tabs[0]-(tabs.size()-1)*discs[lev])/(strens[lev]+custCount_);//th=1
    }

    LMProb getEmitProbFromLast(T emit, LMProb base, const vector<LMProb>& strens, const vector<LMProb>& discs, int lev = -1) const {
        if(lev == -1)
            lev = getLevel();
        if(parent_ != -1)
            base = nodes_[parent_]->getLocalProb(emit, strens[lev-1], discs[lev-1]);// 前一级作 base(x)
        base *= getFallbackProb(strens[lev],discs[lev]);//s,d pitman-yor process parameter 平滑概率
        return base; //(tabs[0]-(tabs.size()-1)*discs[lev])/(strens[lev]+custCount_);//th=1
    }

    LMProb getEmitProbFromLastTwo(T emit, LMProb base, const vector<LMProb>& strens, const vector<LMProb>& discs, int lev = -1) const {
        if(lev == -1)
            lev = getLevel();
        if(parent_ != -1)
            base = nodes_[parent_]->getEmitProbFromLast(emit,base,strens,discs,lev-1);// 前一级作 base(x)
        base *= getFallbackProb(strens[lev],discs[lev]);//s,d pitman-yor process parameter 平滑概率
        return base; //(tabs[0]-(tabs.size()-1)*discs[lev])/(strens[lev]+custCount_);//th=1
    }

    //验证一致性
    bool checkConsistency(const vector< LMProb > & bases, const vector<LMProb>& strens, const vector<LMProb>& discs, double cutoff = 0.0000001, int lev = -1) const {
        if(lev == -1)
            lev = getLevel();
        double totalProb = 0;
        for(T i = 0; i < (T)bases.size(); i++) 
            totalProb += getEmitProb(i,bases[i],strens,discs,lev);//？？？？
        bool consistent = abs(1-totalProb) <= cutoff;
        if(!consistent)
            cerr << "Warning, not consistent (" << 1-totalProb << ")" << endl;
        for(typename NodeMap::const_iterator it = children_.begin(); it != children_.end(); it++)
            nodes_[it->second]->checkConsistency(bases,strens,discs,cutoff,lev+1);
        return consistent;
    }
    
    // add a customer to the appropriate table
    pair<bool,LMProb> addCustomer(T emit, LMProb base, const vector<LMProb>& strens, const vector<LMProb>& discs, int lev) {//word instance=customer;word=dish type; ??can't describe=  table;n-gram=restraunt,a new table means it's exactly producted by last level restraunt,so a new table adds a customer to last level restraunt
        if(emit < 0)
            throw runtime_error("Attempting to add a negative customer, is something wrong?");//
        typename TableMap::iterator it = tables_.find(emit);
        pair<bool,LMProb> ret(false,base);
        if(it == tables_.end()) {
            //cerr << "addCustomer("<<(unsigned)emit<<") -- no tables"<<endl;
            if(parent_ != -1)
                ret = nodes_[parent_]->addCustomer(emit,base,strens,discs,lev-1);    //？？？？
            else {
                //cerr << " --> BASE"<<endl;
                ret.first = true;
            }
            ret.second *= getFallbackProb(strens[lev],discs[lev]);
            vector<int> tabs(2,1);  //tabs[0]
            tables_.insert(pair< T,vector<int> >(emit,tabs));
            tableCount_++;
        }
        else {//fish type is known,so it is exactly a 先验type   P(table=this table)=P(table=this table|dishtype=emit)=P(table=this table)/∑P(table|dishtype=emit)
            // calculate
            vector<int> & tabs = it->second;
            LMProb baseProb = (parent_ == -1?base:nodes_[parent_]->getEmitProb(emit,base,strens,discs,lev-1));
            LMProb totalProb = baseProb * (strens[lev]+tableCount_*discs[lev]) + (tabs[0] - (tabs.size()-1)*discs[lev]);//分子
            ret.second = totalProb/(strens[lev]+custCount_);
            totalProb *= (LMProb)rand()/RAND_MAX;
            int i;
            for(i = tabs.size()-1; i > 0; i--) {//根据CRP过程随机分配到一桌
                totalProb -= (tabs[i]-discs[lev]);
                if(totalProb <= 0)
                    break;
            }

            // generated by a table out of this distribution 新加一张桌子
            if(i == 0) {
                i = tabs.size();
                tabs.push_back(0);
                if(parent_ != -1)
                    ret.first = nodes_[parent_]->addCustomer(emit,base,strens,discs,lev-1).first;
                else
                    ret.first = true;
                tableCount_++;
            }
            //if( i!=tabs.size()-1){i=1;}
            // modify
            tabs[i]++;
            tabs[0]++;
        }
        custCount_++;
        return ret;
    }

    bool removeCustomer(T emit) {
        typename TableMap::iterator it = tables_.find(emit);
        if(it == tables_.end())
            throw runtime_error("Attempt to remove non-existent customer");
        vector<int> & tabs = it->second;
        int i = tabs.size()-1;
        if(tabs.size() > 2) {
            LMProb left = rand()%tabs[0];
            for(; i > 0; i--) {
                left -= tabs[i];
                if(left < 0 )
                    break;
            }
            if(i == 0)
                throw runtime_error("Error in removeCustomer");
        }
        tabs[i]--;
        tabs[0]--;
        custCount_--;

        bool base = false;
        if(tabs[i] == 0) {
            PyNode<T>* myParent = (parent_==-1?0:nodes_[parent_]);
            tableCount_--;
            if(tabs[0] == 0)
                tables_.erase(emit);
            else
                tabs.erase(tabs.begin()+i);
            if(myParent) {
                if(custCount_ == 0)
                    myParent->removeChild(id_);
                base = myParent->removeCustomer(emit);
            }
            else
                base = true;
        }
        return base;
    }

    void removeChild(T emit) {
        typename NodeMap::iterator it = children_.find(emit);
        if(it == children_.end())
            throw runtime_error("Attempt to remove non-existant child");
        delete nodes_[it->second];
        nodes_[it->second] = 0;
        children_.erase(emit);
    }

    PyId findChild(T emit) const {
        typename NodeMap::const_iterator it = children_.find(emit);
        return (it!=children_.end()?it->second:-1);
    }
    
    PyId addChild(T emit) {
        PyId ret = findChild(emit);
        if(ret != -1) return ret;
        ret = nodes_.size();
        children_.insert(pair<T,PyId>(emit,ret));
        nodes_.push_back(new PyNode(nodes_, ret, emit, pos_));
        return ret;
    }
    
    void addCount(CountMap & map, int place) {
        pair<CountMap::iterator,bool> p = map.insert(pair<int,int>(place,0));
        p.first->second++;
    }
    void gatherCounts(CountMap & nodeCustCounts,CountMap & nodeTableCounts,CountMap & tableCustCounts, int & totalCustCount, int & totalTableCount, int lev) {
        if(lev == 0) {
            totalCustCount += custCount_;
            totalTableCount += tableCount_;
            pair<CountMap::iterator,bool> ret;
            if(tableCount_ > 1) {
                addCount(nodeTableCounts,tableCount_);
                addCount(nodeCustCounts,custCount_);
            }
            for(typename TableMap::const_iterator it = tables_.begin(); it != tables_.end(); it++) {
                const vector<int> & tabs = it->second;
                const int tabsize = tabs.size();
                for(int i = 1; i < tabsize; i++)
                    if(tabs[i] > 1)
                        addCount(tableCustCounts,tabs[i]);
            }
        } else {
            for(typename NodeMap::iterator it = children_.begin(); it != children_.end(); it++)
                nodes_[it->second]->gatherCounts(nodeCustCounts,nodeTableCounts,tableCustCounts,totalCustCount,totalTableCount,lev-1);
        }
    }

    const PyId nextContext(T emit) const {//state 切换 用于计算viterbe 与训练无关  a b c d e f //由b切换到c 
        if(parent_ == -1) //root
            return findChild(emit);
        PyId ret = nodes_[parent_]->nextContext(emit), ret2 = -1;
        if(ret != -1 && hasChildren())
            ret2 = nodes_[ret]->findChild(id_);
        return (ret2==-1?ret:ret2);
    }

    T getId() const { return id_; }
    int getCustomerCount() const { return custCount_; }
    int getTableCount() const { return tableCount_; }
    bool hasTable(T id) { return tables_.find(id) != tables_.end(); }
    bool hasChildren() const { return children_.size() != 0; }
    PyNode* getParent() const { return nodes_[parent_]; }
    PyId getPos() const { return pos_; }
    const TableMap & getTables() const { return tables_; }

};

template <class T>
bool operator<(const PyNode<T>& a, const PyNode<T>& b) {
    bool ret;
    if(a.getId() < b.getId())
        ret = true;
    else if(a.getId() > b.getId())
        ret = false;
    else if(b.getParent() == NULL)
        ret = false;
    else if(a.getParent() == NULL)
        ret = true;
    else
        ret = (*a.getParent() < *b.getParent());
    return ret;
}

//std::ofstream ofs1;
template <class T>
class PyLM {
  
private:

    vector<LMProb> discs_, strens_;
    int n_;

    vector<int> basePos_;
    vector< PyNode<T>* > nodes_;//

public:

    void save(ofstream& ofs)
    {
         ofs|n_;
         saveVector(ofs,discs_);
         saveVector(ofs,strens_);
         saveVector(ofs,basePos_);
         size_t size=nodes_.size();
         ofs|size;
         for(size_t i=0;i<nodes_.size();i++)
         {
              nodes_[i]->save(ofs);
              //nodes_[i]->show();
         }
    }

    void load(ifstream& ifs)
    {
         ifs|n_;
         loadVector(ifs,discs_);
         loadVector(ifs,strens_);
         loadVector(ifs,basePos_);
         size_t size;
         ifs|size;
         nodes_.resize(size);
         for(size_t i=0;i<nodes_.size();i++)
         {

             nodes_[i]=new PyNode<T>(nodes_);
             nodes_[i]->load(ifs);
             //nodes_[i]->show();
         }

    }

    // ctor/dtor
    PyLM(int n) : discs_(n,DEFAULT_DISC), strens_(n,DEFAULT_STREN), n_(n), basePos_(), nodes_() {
        nodes_.push_back(new PyNode<T>(nodes_));
       // ofs1.open("log",ios::out|ios::app);
    }
    ~PyLM() {
        for(unsigned i = 0; i < nodes_.size(); i++)
            if(nodes_[i])
                delete nodes_[i];
    }

    // getters/setters
    LMProb getDiscount(int m) { return discs_[m]; }
    LMProb getStrength(int m) { return strens_[m]; }
    const vector<LMProb> & getDiscounts() const { return discs_; }
    const vector<LMProb> & getStrengths() const { return strens_; }
    int getN() { return n_; }
    PyNode<T> & getRoot() { return *nodes_[0]; }
    const PyNode<T> & getRoot() const { return *nodes_[0]; }
    const vector<int> & getBasePositions() { return basePos_; }

    // calculate likelihood/add tables
    LMProb calcSentence(const vector<T> & words, const vector<LMProb> & baseProbs, bool add = true) {
        if(words.size() > baseProbs.size())
            throw runtime_error("word size and base probability size must match in calcSentence");
        return calcSentence(&words[0], &baseProbs[0], words.size(), add);
    }
    vector<vector<T> > reseg(const T* words, const LMProb* baseProbs, ofstream& ofs,int len, bool add = true) const
    {
        //basePos_.clear();
        vector<vector<T> > ret;
        vector<T> nextWord;
        vector<double> bound;
        int i, j;
        LMProb prob = 0;
        for(i = 0; i < len; i++) {
            T emit = words[i];
            //ofs << "calcSentence["<<i<<"] = "<<emit;
            PyId node = 0, next = -1,secondlast=0;
            for(j = 1; j < (int)n_ && i-j >= -1; j++) {
                if(add)
                    next = nodes_[node]->addChild((i-j==-1?0:words[i-j]));
                else
                    next = nodes_[node]->findChild((i-j==-1?0:words[i-j]));
                if(next == -1) break;
                if(j <n_-1) {secondlast=next;}
                node = next;

            }
            if(add) {
                pair<bool,LMProb> res = nodes_[node]->addCustomer(emit, baseProbs[i], strens_, discs_, j-1);
                prob += log(res.second);
                //if(res.first) basePos_.push_back(i);
            } 
            else 
            {
                /*
                ofs<<"\tprob"<<(log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1))-log(nodes_[0]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1)));
                ofs<<"\tone"<<log(nodes_[secondlast]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1))-log(nodes_[0]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1));
                ofs<<"\ttwo"<<log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1))-log(nodes_[secondlast]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1));
                ofs<<"\tori"<<log(nodes_[0]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1));
                ofs<<"\tthr"<<log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1));
                ofs<<"\tfrom3"<<log(nodes_[node]->getLocalProb(emit, strens_[j-1], discs_[j-1]));
                ofs<<"\tfrom2"<<log(nodes_[node]->getEmitProbFromLast(emit,baseProbs[i], strens_, discs_, j-1));
                ofs<<"\tfrom1"<<log(nodes_[node]->getEmitProbFromLastTwo(emit,baseProbs[i], strens_, discs_, j-1))<<endl;
                ofs<<"\tfrom3-from2-0.3"<<log(nodes_[node]->getLocalProb(emit, strens_[j-1], discs_[j-1]))-log(nodes_[node]->getEmitProbFromLast(emit,baseProbs[i], strens_, discs_, j-1))-0.3<<endl;
                */
                //ofs<<"\tthr"<<log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1))<<endl;
                bound.push_back(log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1))-log(nodes_[0]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1)));
            }
            //cout<<"prob"<<prob<<endl;
        }
        bound.push_back(100);
        vector<T> currentWord;
        for(i = 0; i < len; i++) {
             if(currentWord.size()<1)
             {currentWord.push_back(words[i]);}
             else
             {
                 if(currentWord.size()==1)
                 {
                    if(bound[i]<bound[i+1]-2)
                    {
                         if(bound[i]<bound[i-1]-1||(bound[i]<2.2&&i==1))
                         {

                                ret.push_back(currentWord);
                                currentWord.clear();
                                currentWord.push_back(words[i]);
                         }
                         else
                         {
                                currentWord.push_back(words[i]);
                         }
                    }  
                    else
                    {
                         currentWord.push_back(words[i]);
                    }
                 }
                 else
                 {  
                    if(i==len-1&&bound[i]>bound[i-1]-2.5)
                    {
                         currentWord.push_back(words[i]);
                    }
                    else if((bound[i]<bound[i+1]-3)||(bound[i]<bound[i-1]-3))
                    {
                         ret.push_back(currentWord);
                         currentWord.clear();
                         currentWord.push_back(words[i]);
                    }
                    else if(bound[i]<bound[i+1]-1)
                    {
                         if(bound[i]<bound[i-1]-1||(bound[i]<2&&i==1))
                         {

                                ret.push_back(currentWord);
                                currentWord.clear();
                                currentWord.push_back(words[i]);
                         }
                         else
                         {
                                currentWord.push_back(words[i]);
                         }
                    }  
                    else
                    {
                         currentWord.push_back(words[i]);
                    }
                 }
             }
        }
        if(currentWord.size()>0)
        {
             ret.push_back(currentWord);
        }
        return ret;
/**/
    }
    LMProb calcSentence(const T* words, const LMProb* baseProbs, int len, bool add = true) {
        basePos_.clear();
        int i, j;
        LMProb prob = 0;
        for(i = 0; i < len; i++) {
            T emit = words[i];
            //cerr << "calcSentence["<<i<<"] = "<<emit<<endl;
            PyId node = 0, next = -1;
            // get the total probability
            for(j = 1; j < (int)n_ && i-j >= -1; j++) {
                if(add)
                    next = nodes_[node]->addChild((i-j==-1?0:words[i-j]));
                else
                    next = nodes_[node]->findChild((i-j==-1?0:words[i-j]));
                if(next == -1) break;
                node = next;
            }
            if(add) {
                pair<bool,LMProb> res = nodes_[node]->addCustomer(emit, baseProbs[i], strens_, discs_, j-1);
                prob += log(res.second);
                if(res.first) basePos_.push_back(i);
            } 
            else 
                prob += log(nodes_[node]->getEmitProb(emit,baseProbs[i], strens_, discs_, j-1));
        }
        return prob;
    }
    
    // remove tables
    void removeCustomers(const vector<T> & words) {
        return removeCustomers(&words[0], words.size());
    }
    void removeCustomers(const T* words, int len) {
        basePos_.clear();
        int i, myN;
        for(i = 0; i < len; i++) {
            T emit = words[i];
            PyId node = 0;
            for(myN = 1; myN < (int)n_ && i-myN >= -1; myN++) {
                node = nodes_[node]->findChild(i-myN==-1?0:words[i-myN]);
                if(node == -1) { 
                    throw runtime_error("Couldn't find node to be deleted");
                }
            }
            if(nodes_[node]->removeCustomer(emit))
                basePos_.push_back(i);
        }
    }

    // print lm
    void print(const string* strs, const LMProb* bases, ostream & out = cout) const { 
        vector<unsigned> counts(n_);
        nodes_[0]->accumulateCounts(counts,0);
        out << "[unifb]" << endl << 
            log(nodes_[0]->getFallbackProb(strens_[0], discs_[0]))/log(10) << endl << endl <<
            "\\data\\" << endl;
        for(int i = 0; i < n_; i++)
            out << "ngram "<<i+1<<"="<<counts[i]<<endl;
        for(int i = 0; i < n_; i++) {
            out << endl << "\\" << i+1 << "-grams:" << endl;
            nodes_[0]->print(0,i,strs,bases,strens_,discs_,out);
        }
    }


   void compute(const string* strs, const LMProb& base)  { 
        for(int i = 0; i < n_; i++) {
            nodes_[0]->compute(0,i,strs,base,strens_,discs_);
        }
    }

    // auxiliary variables method  one step for gibbs sampling
    void sampleParameters() {
        for(int i = n_-1; i >= 0; i--) {
            LMProb stren = strens_[i], disc = discs_[i];
            CountMap nodeTableCounts, nodeCustCounts, tableCustCounts;
            int totalCustCount = 0, totalTableCount = 0;
            nodes_[0]->gatherCounts(nodeCustCounts,nodeTableCounts,tableCustCounts,totalCustCount,totalTableCount,i);
            LMProb da = PRIOR_DA, db = PRIOR_DB, sa = PRIOR_SA, sb = PRIOR_SB;
            int yui = 0;
            for(CountMap::const_iterator it = nodeTableCounts.begin(); it != nodeTableCounts.end(); it++) {
                for(int j = 1; j < it->first; j++) {
                    for(int k = 0; k < it->second; k++) {
                        yui = bernoulliSample(stren/(stren+disc*j));
                        da += (1-yui);
                        sa += yui;
                    }
                }
            }
            for(CountMap::const_iterator it = nodeCustCounts.begin(); it != nodeCustCounts.end(); it++)
                for(int k = 0; k < it->second; k++)
                    sb -= log(betaSample(stren+1,it->first-1));
            for(CountMap::const_iterator it = tableCustCounts.begin(); it != tableCustCounts.end(); it++)
                for(int j = 1; j < it->first; j++)
                    for(int k = 0; k < it->second; k++)
                        db += (1-bernoulliSample((j-1)/(j-disc)));
            discs_[i] = betaSample(da,db);
            strens_[i] = gammaSample(sa,1/sb);
        }
    }

    unsigned getVocabSize() const { return nodes_[0]->getTables().size(); }
    unsigned size() const { return nodes_.size(); }
    PyNode<T>* getNode(unsigned id) { return nodes_[id]; }
    const PyNode<T>* getNode(unsigned id) const { return nodes_[id]; }

    // reduce the states and vocabulary
    //  return the vocabulary map
    vector<T> trim(bool trimVocab = true) {
        // trim the vocabulary ids
        vector<T> nextVocab;
        T nextWord = 1;
        nextVocab.push_back(0);
        const typename PyNode<T>::TableMap & tm = nodes_[0]->getTables();
        for(typename PyNode<T>::TableMap::const_iterator it = tm.begin();
                it != tm.end(); it++) {
            if(trimVocab) {
                if((T)nextVocab.size() <= it->first)
                    nextVocab.resize(it->first+1,-1);
                if(nextVocab[it->first] == -1)
                    nextVocab[it->first] = nextWord++;
            } 
            else 
                while((T)nextVocab.size() <= it->first)
                    nextVocab.push_back(nextVocab.size());
        }
        // get the new node ids
        vector<int> nextIds(nodes_.size(), -1);
        vector< PyNode<T>* > nextNodes;
        for(unsigned i = 0; i < nodes_.size(); i++) {
            if(nodes_[i] != 0) {
                nextIds[i] = nextNodes.size();
                nextNodes.push_back(nodes_[i]);
            }
        }
        nodes_ = nextNodes;
        // trim each node
        for(unsigned i = 0; i < nextNodes.size(); i++)
            nextNodes[i]->trim(nextIds, nextVocab);
        return nextVocab;
    }


private:

    ///////////////////////////////
    // begin numerical functions //
    ///////////////////////////////
    
    // distribution sampling functions
   

//取样概率密度函数 = 概率密度函数* 反函数的倒数  P（y<f(a)）=P(x<a)  [P（y<f(a)）-P（y<f(b)）]/[f(a)-f(b)]=[P(x<a)-P(x<b)]/(a-b)   *  (a-b)/(f(a)-f(b))

   //贝努利取样
    static int bernoulliSample(LMProb p) {
        return (rand() < p*RAND_MAX?1:0);
    }

   //γ取样
    static LMProb gammaSample(LMProb a, LMProb scale) {
        LMProb b, c, e, u, v, w, y, x, z;
        if(a > 1) { // Best's XG method
            b = a-1;
            c = 3*a-.75;
            bool accept = false;
            do {
                u = (LMProb)rand()/RAND_MAX;
                v = (LMProb)rand()/RAND_MAX;
                w = u*(1-u);
                y = sqrt(c/w)*(u-.5);
                x = b+y;
                if(x >= 0) {
                    z = 64*w*w*w*v*v;
                    accept = (z <= 1-2*y*y/x || log(z) <= 2*(b*log(x/b)-y));
                }
            } while (!accept);
        } else { // Johnk's method
            do {
                u = (LMProb)rand()/RAND_MAX;
                v = (LMProb)rand()/RAND_MAX;
                x = pow(u,1/a);
                y = pow(v,1/(1-a));
            } while (x+y > 1);
            e = exponSample(1);
            x = e*x/(x+y);
        }
        return x * scale;
    }

    //β取样
    static LMProb betaSample(LMProb a, LMProb b) {
        LMProb ga = gammaSample(a,1);
        LMProb gb = gammaSample(b,1);
        return ga/(ga+gb);
    }

    //指数函数取样
    static LMProb exponSample(LMProb l) {
        return -1*log(1-(LMProb)rand()/RAND_MAX)/l;
    }

    LMProb betaLogDensity(LMProb x, LMProb a, LMProb b) {
        LMProb ret =  log(tgamma(a+b)/tgamma(a)/tgamma(b)*pow(x,a-1)*pow(1-x,b-1));
        return ret;
    }
    LMProb gammaLogDensity(LMProb x, LMProb k, LMProb t) {
        LMProb ret = log(pow(x,k-1)*exp(-1*x/t)/pow(t,k)/tgamma(k));
        return ret;
    }
    
};

}

#endif

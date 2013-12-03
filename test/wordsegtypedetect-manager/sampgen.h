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


#ifndef SAMPGEN_H__
#define SAMPGEN_H__

#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <vector>
#include <stdexcept>
#include "util.h"

namespace fst {

// sample a single value appropriately from a vector of weights
unsigned SampleWeights(vector<float> & ws, float anneal = 1) {//caculate predictive probility 论文3.24
    bool last=false;
    if(anneal>100) {last=true;anneal=1;}
    if(ws.size() == 0)
        throw runtime_error("No final states found during sampling");
    else if(ws.size() == 1)
        return 0;

    float minWeight = numeric_limits<float>::infinity(), weightTotal = 0;
    unsigned i;
    unsigned mini=0;
    for (i = 0; i < ws.size(); i++) {
        ws[i] *= anneal;
        if(last&&ws[i]<minWeight){mini=i;}
        minWeight = min(ws[i], minWeight);//
    }
    if(last){return mini;}
    for (i = 0; i < ws.size(); i++) {//Gibbs sampling but 舍弃了过低的项 
        float & f = ws[i];
        f = exp(minWeight-f);
        weightTotal += f;
    }
    // cout << "Total weight=" << weightTotal;
    weightTotal *= rand()/(double)RAND_MAX;
    // cout << ", random weight=" << weightTotal << " (basis " << minWeight << ")"<<endl;
    for(i = 0; i < ws.size(); i++) {
        weightTotal -= ws[i];
        // cout << " after weight " << i << ", " << weightTotal << endl;
        if(weightTotal <= 0)
            break;
    }
    if(i == ws.size()) {
        cerr << "WARNING: Sampling failed, probability mass left at end of cycle";
        i--;
    }
    return i;
}

template<class A>
void SampGen(const Fst<A> & ifst, MutableFst<A> & ofst, const string*  symbols,const string*  symbols2,int numchar,unsigned nbest = 1, float anneal = 1) 
{ //sample generate 
    //cout<<"ifst"<<ifst.size()<<endl;
    //cout<<"ofst"<<ofst.size()<<endl;
    typedef Fst<A> F;
    typedef typename F::Weight W;
    typedef typename A::StateId S;

    // sanity check
    if(ifst.Final(ifst.Start()) != numeric_limits<float>::infinity())
        throw runtime_error("Sampling FSTs where start states are final is not supported yet");

    // the number of remaining incoming arcs, and total weights of each state
    std::vector< int > incomingArcs;
    std::vector< vector< A > > backArcs;
    std::vector< W > stateWeights;
    unsigned i, statesFinished = 0;
    
    // intialize the data values
    for (StateIterator< Fst<A> > siter(ifst); !siter.Done(); siter.Next()) {
        S s = siter.Value();
        //cout << "state: " << s << endl;
        for(ArcIterator< F > aiter(ifst, s); !aiter.Done(); aiter.Next()) {
            const A& a = aiter.Value();
            // cout << " -> " << a.nextstate ;
            while((unsigned)a.nextstate >= incomingArcs.size()) {
                incomingArcs.push_back(0);
                stateWeights.push_back(W::Zero());
                backArcs.push_back(std::vector<A>());
            }
            incomingArcs[a.nextstate]++;
            backArcs[a.nextstate].push_back(A(a.ilabel, a.olabel, a.weight, s));
            //cout<<" \tilabel:"<<symbols[a.ilabel].substr(1)<<"\tolabel:";
            /*
            if(a.olabel>numchar+2)
               cout<<symbols[a.olabel-numchar];
            else
               cout<<a.olabel;//symbols2[
             */
            //cout<<"\tweight:"<<a.weight<< endl;
        }
    }
    SafeAccess(stateWeights, ifst.Start()) = W::One();
    SafeAccess(incomingArcs, ifst.Start()) = 0;

    // calculate the number of arcs incoming to each state
    vector< S > stateQueue(1,ifst.Start());
    while(stateQueue.size() > 0) {//类似viterbe，backward,但不是取最高概率，而是根据概率随机取样,每个状态往前按概率回溯取样，gibbs sampling的1个step,另一个step是add customer，还有一个step是sampleParameters是整体的一个参数取样
        unsigned s = stateQueue[stateQueue.size()-1];
        stateQueue.pop_back();
        for(ArcIterator< F > aiter(ifst, s); !aiter.Done(); aiter.Next()) {
            const A& a = aiter.Value();
            // cout << "stateWeights[" << a.nextstate << "]: min(" << stateWeights[a.nextstate]<<",("<<stateWeights[s]<<"+"<<a.weight<<"))"<<endl;
            stateWeights[a.nextstate] = Plus(stateWeights[a.nextstate],Times(stateWeights[s],a.weight));
            // cout << " -> " << stateWeights[a.nextstate] << endl;
            if(--incomingArcs[a.nextstate] == 0)
                stateQueue.push_back(a.nextstate);
        }
        statesFinished++;
    }
    if(statesFinished != incomingArcs.size())
        throw std::runtime_error("Sampling cannot be performed on cyclic FSTs");


    // sample the states backwards from the final state
    ofst.AddState();
    ofst.SetStart(0);

    for(unsigned n = 0; n < nbest; n++) {//nbest=1

        // find the final states and sample a final state
        vector< float > stateCandWeights;//候选state
        vector< S > stateCandIds;
        for (StateIterator< Fst<A> > siter(ifst); !siter.Done(); siter.Next()) {
            S s = siter.Value();
            float w = Times(ifst.Final(s),stateWeights[s]).Value();
            if(w != numeric_limits<float>::infinity()) {
                 //cout << "Final state "<<s<<","<<w<<endl;
                stateCandWeights.push_back( w );
                stateCandIds.push_back( s );
            }
        }
        S currState = stateCandIds[SampleWeights(stateCandWeights, anneal)];//no use,stateCandWeights.size()=1

        // add the final state
        S outState = (ifst.Start() != currState?ofst.AddState():0);
        ofst.SetFinal(outState, ifst.Final(currState));

        // sample the values in order
        while(outState != 0) {
            const vector<A> & arcs = backArcs[currState];
            vector<float> arcWeights(arcs.size(), 0);
            for(i = 0; i < arcs.size(); i++) 
                arcWeights[i] = Times(arcs[i].weight,stateWeights[arcs[i].nextstate]).Value();
            const A & myArc = arcs[SampleWeights(arcWeights, anneal)];
            S nextOutState = (myArc.nextstate != ifst.Start()?ofst.AddState():0);
           // cout << "Adding arc " << nextOutState << "--"<<myArc.ilabel<<"/"<<myArc.olabel<<":"<<myArc.weight<<"-->"<<outState<<endl;
            ofst.AddArc(nextOutState, A(myArc.ilabel,myArc.olabel,myArc.weight,outState));
            outState = nextOutState;
            currState = myArc.nextstate;
        }
    
    }

}

}

#endif

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


#ifndef __PYLM_FST_
#define __PYLM_FST_

#include "pylm.h"
#include <fst/fst.h>

#define PHI_SYMBOL 1

using namespace pylm;

namespace fst {

template< class WordId, class CharId >
class PylmFst : public Fst<StdArc> {
    
public:

    static const int kFileVersion = 1;

    typedef typename StdArc::StateId StateId;
    typedef typename StdArc::Weight Weight;

    const PyLM<WordId> * knownLm_;
    const PyLM<CharId> * unkLm_;

    const int unkVocabSize_;
    const double unkBase_;

    mutable vector< vector< StdArc >* > arcs_;
    string type_;
    uint64 properties_;

    PylmFst(const PyLM<WordId> & knownLm, const PyLM<CharId> & unkLm, unsigned unkVocabSize) :
        knownLm_(&knownLm), unkLm_(&unkLm), 
        unkVocabSize_(unkVocabSize), unkBase_(1.0/unkVocabSize_),
        arcs_(knownLm.size()+unkLm.size(), 0), type_("vector"), 
        properties_(kOEpsilons | kILabelSorted | kOLabelSorted) {
        if(!PHI_SYMBOL) properties_ |= kIEpsilons;
    }
    
    ~PylmFst() {
        for(StateId i = 0; i < (StateId)arcs_.size(); i++)
            if(arcs_[i])
                delete arcs_[i];
    }

    StateId Start() const {
        return max(knownLm_->getRoot().findChild(0),0);
    }

    Weight Final(StateId stateId) const {//Pbase()
        return stateId < (StateId)knownLm_->size() ? Weight::One() : Weight::Zero();
    }

    template <class T>
    double BuildArcs(const PyLM<T> & pylm, double base, //建立state->probility表 类似vertibe forward-filtering
                        StateId stateId, WordId vocabSize,
                        vector<StdArc>* logs) const {
        typedef typename PyNode<T>::TableMap TableMap;
        const PyNode<T>* myNode = pylm.getNode(stateId);
        if(!myNode) return 1;
        // expand all values for the first state only
        const TableMap & myTables = myNode->getTables();
        // add the actual weights
        double fallback = 1;
        if(stateId == 0) {
            for(WordId id = 0; id < vocabSize; id++) {
                StateId next = myNode->nextContext(id);
                if(next == -1) next = 0;
                double prob = myNode->getEmitProb(id,base,pylm.getStrengths(),pylm.getDiscounts());
                if(prob != 0) {
                    fallback -= prob;
                    logs->push_back(StdArc(id+2,id+2,TropicalWeight(-1*log(prob)),next));
                }
            }
        } else if(myTables.size() > 0) {
            logs->reserve(myTables.size()+1);
            logs->push_back(StdArc(PHI_SYMBOL,0,TropicalWeight(0),myNode->getParent()->getPos()));
            for(typename TableMap::const_iterator it = myTables.begin(); it != myTables.end(); it++) {
                StateId id = it->first;
                StateId next = myNode->nextContext(id);
                if(next == -1) next = 0;
                double prob = myNode->getEmitProb(id,base,pylm.getStrengths(),pylm.getDiscounts());
                fallback -= prob;
                logs->push_back(StdArc(id+2,id+2,TropicalWeight(-1*log(prob)),next));
            }
            (*logs)[0].weight = TropicalWeight(-1*log(fallback));
        }
        return fallback;
    }

    const vector<StdArc> * GetArcs(StateId stateId) const {
        if(stateId < 0 || stateId >= (StateId)arcs_.size())
            throw runtime_error("PylmFst::GetArcs: StateId is out of bounds");
        if(arcs_[stateId] == NULL) {
            vector<StdArc> * logs = new vector<StdArc>;
            double fallback = 0;
            // known LM state
            const StateId kSize = knownLm_->size();
            if(stateId < kSize) {
                // make an extra fallback to the unknown words if it's the home state
                if(stateId == 0) {
                    unsigned id = max(unkLm_->getRoot().findChild(0),0)+kSize;
                    logs->push_back(StdArc(PHI_SYMBOL,0,TropicalWeight(0),id));
                    fallback = BuildArcs(*knownLm_, 0, stateId, knownLm_->getVocabSize(), logs);
                    (*logs)[0].weight = TropicalWeight(-1*log(fallback));
                }
                else
                    BuildArcs(*knownLm_, 0, stateId, knownLm_->getVocabSize(), logs);
                // increase the sizes appropriately
                for(unsigned i = 0; i < logs->size(); i++) {
                    StdArc & arc = (*logs)[i];
                    if(arc.ilabel > 1) arc.ilabel += unkVocabSize_;
                    if(arc.olabel > 1) arc.olabel += unkVocabSize_;
                }
            }
            // unknown LM state
            else {
                fallback = BuildArcs(*unkLm_, unkBase_, stateId-kSize, unkVocabSize_, logs);
                for(unsigned i = 0; i < logs->size(); i++) {
                    StdArc & arc = (*logs)[i];
                    // the unknown word terminal symbol returns the base state
                    if(arc.olabel == 3) arc.nextstate = 0;
                    // all other states are moved to the right
                    else arc.nextstate += kSize;
                }
            }
            arcs_[stateId] = logs;
        }
        return arcs_[stateId];
    }

    size_t NumArcs(StateId stateId) const {
        return GetArcs(stateId)->size();
    }

    size_t NumInputEpsilons(StateId stateId) const {
        if(PHI_SYMBOL != 0)
            return 0;
        return (stateId==(StateId)knownLm_->size()?0:1);
    }

    size_t NumOutputEpsilons(StateId stateId) const {
        // only the unknown word model base
        //  state doesn't have epsilons, all others have exactly one
        //  (as the fallback)
        return (stateId==(StateId)knownLm_->size()?0:1);
    }

    uint64 Properties(uint64 mask, bool test) const {
        return mask & properties_;
    }

    const std::string& Type() const {
        return type_;
    }

    PylmFst<WordId, CharId>* Copy(bool reset = false) const {
        return new PylmFst<WordId, CharId> (*knownLm_, *unkLm_, unkVocabSize_);
    }

    const fst::SymbolTable* InputSymbols() const {
        // unimplemented, as symbols cannot be specified
        return NULL;
    }

    const fst::SymbolTable* OutputSymbols() const {
        // unimplemented, as symbols cannot be specified
        return NULL;
    }

    void InitStateIterator(fst::StateIteratorData<StdArc>* data) const {
        data->base = 0;
        data->nstates = arcs_.size();
    }

    void InitArcIterator(StateId stateId, fst::ArcIteratorData<StdArc>* data) const {
        data->base = 0;
        const vector<StdArc> * myArcs = GetArcs(stateId);
        data->narcs = myArcs->size();
        data->arcs = data->narcs > 0 ? &((*myArcs)[0]) : 0;
        data->ref_count = 0;
    }

    // Write a VectorFst to a file; return false on error
    // Empty filename writes to standard output
    virtual bool Write(const string &filename) const {
        if (!filename.empty()) {
            ofstream strm(filename.c_str(), ofstream::out | ofstream::binary);
            if (!strm) 
                throw runtime_error("PylmFst::Write: Can't open file");
            return Write(strm, FstWriteOptions(filename));
        } else
            return Write(std::cout, FstWriteOptions("standard output"));
    }

    bool Write(ostream &strm, const FstWriteOptions &opts) const {
        FstHeader hdr;
        hdr.SetStart(Start());
        hdr.SetNumStates(arcs_.size());
        WriteHeader(strm, opts, kFileVersion, &hdr);
    
        for (StateId s = 0; s < (StateId)arcs_.size(); ++s) {
            // const VectorState<A> *state = GetState(s);
            Final(s).Write(strm);
            //int64 narcs_ = state->arcs_.size();
            const vector<StdArc> * myArcs = GetArcs(s);
            int64 narcs_ = myArcs->size();
            WriteType(strm, narcs_);
            for (size_t a = 0; a < myArcs->size(); ++a) {
                const StdArc &arc = (*myArcs)[a];
                WriteType(strm, arc.ilabel);
                WriteType(strm, arc.olabel);
                arc.weight.Write(strm);
                WriteType(strm, arc.nextstate);
            }
        }
    
        strm.flush();
        if (!strm) 
            throw std::runtime_error("PylmFst::Write: write failed");
        return true;
    }

    // Write-out header and symbols from output stream.
    // If a opts.header is false, skip writing header.
    // If opts.[io]symbols is false, skip writing those symbols.
    void WriteHeader(ostream &strm, const FstWriteOptions& opts,
                     int version, FstHeader *hdr) const {
        if (opts.write_header) {
            hdr->SetFstType(type_);
            hdr->SetArcType(StdArc::Type());
            hdr->SetVersion(version);
            hdr->SetProperties(properties_);
            // int32 file_flags = 0;
            // if (isymbols_ && opts.write_isymbols)
            //     file_flags |= FstHeader::HAS_ISYMBOLS;
            // if (osymbols_ && opts.write_osymbols)
            //     file_flags |= FstHeader::HAS_OSYMBOLS;
            // hdr->SetFlags(file_flags);
            hdr->SetFlags(0);
            hdr->Write(strm, opts.source);
        }
        // if (isymbols_ && opts.write_isymbols) isymbols_->Write(strm);
        // if (osymbols_ && opts.write_osymbols) osymbols_->Write(strm);
    }

};

}

#endif

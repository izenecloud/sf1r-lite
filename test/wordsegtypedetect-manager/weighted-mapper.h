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

#ifndef _WEIGHTED_MAP_H__
#define _WEIGHTED_MAP_H__

#include <fst/fst.h>
#include <fst/map.h>
#include <fst/arc.h>

namespace fst {

// a map putting a log-linear weight on the arc value
struct WeightedMapper {

    float weight_;
    
    WeightedMapper(float weight) : weight_(weight) { }

    StdArc operator()(const StdArc &arc) const {
        TropicalWeight ret( arc.weight.Value() *
            ( arc.weight.Value() == FloatLimits<float>::PosInfinity() ? 1 : weight_ ) );
        return StdArc(arc.ilabel, arc.olabel, ret, arc.nextstate);
    }

    MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

    MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

    MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

    uint64 Properties(uint64 props) const { return props; }

};

}

#endif

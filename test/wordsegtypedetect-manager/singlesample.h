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


#ifndef SINGLE_SAMPLE_H
#define SINGLE_SAMPLE_H

#include <vector>

namespace latticelm {

typedef int SentId;
typedef short CharId;
typedef int WordId;

class SingleSample {
    
public:
    SentId sentId;
    std::vector<CharId> surf;
    std::vector<bool> boundary;
    SingleSample() : sentId(-1), surf(), boundary() { }

};

}

#endif

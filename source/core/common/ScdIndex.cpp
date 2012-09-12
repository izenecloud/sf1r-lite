/* 
 * File:   ScdIndex.cpp
 * Author: Paolo D'Apice
 * 
 * Created on September 11, 2012, 11:39 AM
 */

#undef NDEBUG
#include "ScdIndex.h"
#include "ScdParser.h"
#include <glog/logging.h>

namespace scd {

const int log_interval = 1000;

ScdIndex*
ScdIndex::build(const std::string& path) {
    static ScdParser parser;
    typedef ScdParser::iterator iterator;
    
    CHECK(parser.load(path)) << "Cannot load file: " << path;
    LOG(INFO) << "Building index on: " << path << " ...";
   
    ScdIndex* index = new ScdIndex;
    iterator end = parser.end();
    for (iterator it = parser.begin(); it != end; ++it) {
        SCDDocPtr doc = *it;
        CHECK(doc) << "Document is null";
        
        DLOG(INFO) << "got document '" << doc->at(0).second << "' @ " << it.getOffset();

        index->container.insert(scd::Document<>(it.getOffset(), doc));
        LOG_EVERY_N(INFO, log_interval) << "Saved " << google::COUNTER << " documents ...";
    }
    
    LOG(INFO) << "Indexed " << index->size() << " documents.";

    return index;
}

} /* namespace scd */
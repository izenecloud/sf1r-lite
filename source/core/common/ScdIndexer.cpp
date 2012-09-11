/* 
 * File:   ScdIndexer.cpp
 * Author: Paolo D'Apice
 * 
 * Created on September 11, 2012, 11:39 AM
 */

#include "ScdIndexer.h"
#include <glog/logging.h>

namespace {
const int log_interval = 1000;
}

ScdIndexer::ScdIndexer() {
}

ScdIndexer::~ScdIndexer() {
}

bool
ScdIndexer::build(const std::string& path) {
    CHECK(parser.load(path)) << "Cannot load file: " << path;

    LOG(INFO) << "Building index on: " << path << " ...";
    
    typedef ScdParser::iterator iterator;
    iterator end = parser.end();
    for (iterator it = parser.begin(); it != end; ++it) {
        SCDDocPtr doc = *it;
        
        DLOG(INFO) << "got document:" << (*doc)[0].second; 
        DLOG(INFO) << "at offset:" << it.getOffset();

        index.insert(scd::Document<>((*doc)[0].second, it.getOffset()));
        LOG_EVERY_N(INFO, log_interval) << "Saved " << google::COUNTER << " documents ...";
    }
    LOG(INFO) << "Indexed " << index.size() << " documents.";

    return true;
}
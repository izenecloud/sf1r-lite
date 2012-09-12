/* 
 * File:   ScdIndexer.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 11:39 AM
 */

#ifndef SCDINDEXER_H
#define	SCDINDEXER_H

#include "ScdParser.h"
#include "ScdIndex.h"
#include <boost/noncopyable.hpp>
#include <string>


class ScdIndexer : boost::noncopyable {
public:
    ScdIndexer();
    ~ScdIndexer();

    bool build(const std::string& path);
    
//    void save(const std::string& file) const {
//        index.save(file);
//    }    
//
//    void load(const std::string& file) {
//        index.load(file);
//    }
    
    const scd::ScdIndex& getIndex() const {
        return index;
    }
    
private:
    ScdParser parser;
    scd::ScdIndex index;
};

#endif	/* SCDINDEXER_H */
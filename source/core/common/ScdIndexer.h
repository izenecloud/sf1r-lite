/* 
 * File:   ScdIndexer.h
 * Author: Paolo D'Apice
 *
 * Created on September 11, 2012, 11:39 AM
 */

#ifndef SCDINDEXER_H
#define	SCDINDEXER_H

#include "ScdParser.h"
#include <boost/noncopyable.hpp>
#include <string>


class ScdIndexer : boost::noncopyable {
public:
    ScdIndexer();
    ~ScdIndexer();

    bool build(const std::string& path);

private:
    ScdParser parser;
};

#endif	/* SCDINDEXER_H */
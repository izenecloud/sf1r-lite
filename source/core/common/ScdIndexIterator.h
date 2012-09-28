/*
 * File:   ScdIndexIterator.h
 * Author: Paolo D'Apice
 *
 * Created on September 27, 2012, 5:10 PM
 */

#ifndef SCDINDEX_ITERATOR_H
#define SCDINDEX_ITERATOR_H

#include "ScdParser.h"
#include <glog/logging.h>
#include <iterator>

namespace scd {

/// Iterator on the SCD index.
class ScdIndexIterator : public std::iterator<std::input_iterator_tag, SCDDoc> {
    template <typename, typename> friend class ScdIndex;
    typedef ScdParser::iterator ParserIterator;

    ScdIndexIterator(ScdParser* p, const offset_type& offset) :
            parser(p), tail(false) {
        setDocument(offset);
    }

    ScdIndexIterator(ScdParser* p, const offset_list& list) :
            parser(p), tail(true), offsets(list) {
        if (not offsets.empty()) {
            tail = false;
            current = offsets.begin();
            setDocument(*current);
        }
    }

public:
    /// Default constructor.
    ScdIndexIterator() : tail(true) {}

    /// Copy constructor.
    ScdIndexIterator(const ScdIndexIterator& it) :
            parser(it.parser), tail(it.tail), offsets(it.offsets),
            current(it.current), document(it.document) {}

    /// Destructor.
    ~ScdIndexIterator() {}

    /// Post-increment.
    ScdIndexIterator& operator++() {
        if (offsets.empty() or ++current == offsets.end()) {
            tail = true;
        } else {
            setDocument(*current);
        }
        return *this;
    }

    /// Pre-increment.
    ScdIndexIterator operator++(int) {
        ScdIndexIterator tmp(*this);
        operator++();
        return tmp;
    }

    /// Equality.
    bool operator==(const ScdIndexIterator& it) const {
        return (tail and it.tail)
            or (tail == it.tail and *document == *document);
    }

    /// Disequality.
    bool operator!=(const ScdIndexIterator& it) const {
        return not operator==(it);
    }

    /// Dereference.
    const SCDDoc& operator*() const {
        return *document;
    }

private:
    void setDocument(const offset_type offset) {
        DLOG(INFO) << "getting document @ " << offset;
        if (parser->fs_.eof())
            parser->fs_.clear();
        parser->fs_.seekg(offset, ios::beg);
        ParserIterator it(parser, 0);
        document = *it;
        CHECK(document) << "document is NULL (offset=" << offset << ")";
    }

private:
    ScdParser* parser;
    bool tail;
    offset_list offsets;
    offset_list::iterator current;
    SCDDocPtr document;
};

} /* namespace scd */

#endif /* SCDINDEX_ITERATOR_H */

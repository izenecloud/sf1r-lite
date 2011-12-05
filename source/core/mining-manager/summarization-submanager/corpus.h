/*
 * corpus.h
 *
 *  Created on: 2010-10-18
 *      Author: jay
 */

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_

#include <util/ustring/UString.h>

#include <boost/unordered_map.hpp>
#include <vector>

using izenelib::util::UString;

namespace sf1r
{

class Corpus
{
public:
    Corpus();

    ~Corpus();

    inline int ntotal() const
    {
        return _word_seqs.size();
    }

    inline int nwords() const
    {
        return _map_id2str.size();
    }

    inline int nsents() const
    {
        return _sent_offs.size() - 1;
    }

    inline int ndocs() const
    {
        return _doc_offs.size() - 1;
    }

    inline int ncolls() const
    {
        return _coll_offs.size() - 1;
    }

    inline const int* get_word_seq() const
    {
        if (_word_seqs.empty())
            return NULL;

        return &_word_seqs[0];
    }

    inline const int* get_sent_offs() const
    {
        if (_sent_offs.empty())
            return NULL;

        return &_sent_offs[0];
    }

    inline const int* get_doc_offs() const
    {
        if (_doc_offs.empty())
            return NULL;

        return &_doc_offs[0];
    }

    inline const int* get_coll_offs() const
    {
        if (_coll_offs.empty())
            return NULL;

        return &_coll_offs[0];
    }

    void reset();

    void start_new_sent();

    void start_new_sent(const UString& sent);

    void start_new_doc();

    void start_new_coll();

    void start_new_coll(const UString& name);

    const UString& get_word(int w) const;

    const UString& get_sent(int s) const;

    const UString& get_coll_name(int c) const;

    void add_word(const UString word);

    void add_sent(const UString& sent);

private:
    std::vector<int> _word_seqs;
    std::vector<int> _sent_offs;
    std::vector<int> _doc_offs;
    std::vector<int> _coll_offs;

    std::vector<UString> _sents;
    std::vector<UString> _coll_names;

    boost::unordered_map<UString, int> _map_str2id;
    std::vector<UString> _map_id2str;
};

}

#endif /* SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_ */

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
        return _sent_offs.size();
    }

    inline int ndocs() const
    {
        return _doc_offs.size();
    }

    inline int ncolls() const
    {
        return _coll_offs.size();
    }

    inline const int* get_doc_offs() const
    {
        return &_doc_offs.at(0);
    }

    inline const int* get_word_seq() const
    {
        return &_word_seqs.at(0);
    }

    inline const int* get_sent_offs() const
    {
        return &_sent_offs.at(0);
    }

    inline const int* get_coll_offs() const
    {
        return &_coll_offs.at(0);
    }

    void reset();

    void start_new_sent();

    void start_new_doc();

    void start_new_coll();

    const izenelib::util::UString& get_sent(int s) const;

    void add_word(const izenelib::util::UString word);

    void add_sent(const izenelib::util::UString& sent);

    void add_doc(const izenelib::util::UString& doc);

private:
    int _word_pos;
    std::vector<int> _word_seqs;
    std::vector<int> _sent_offs;
    std::vector<int> _doc_offs;
    std::vector<int> _coll_offs;

    std::vector<izenelib::util::UString> _sents;

    boost::unordered_map<izenelib::util::UString, int> _map_str2id;
    std::vector<izenelib::util::UString> _map_id2str;
};

}

#endif /* SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_ */

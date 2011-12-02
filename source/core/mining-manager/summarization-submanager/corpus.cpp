#include "corpus.h"

using izenelib::util::UString;

namespace sf1r
{

Corpus::Corpus()
    : _word_pos(0)
{
}

Corpus::~Corpus()
{
}

void Corpus::reset()
{
    _word_pos = 0;
    _word_seqs.clear();
    _sent_offs.clear();
    _doc_offs.clear();
    _coll_offs.clear();
    _sents.clear();
    _map_str2id.clear();
    _map_id2str.clear();
}

void Corpus::start_new_sent()
{
    _sent_offs.push_back(_word_pos);
}

void Corpus::start_new_doc()
{
    _doc_offs.push_back(_word_pos);
}

void Corpus::start_new_coll()
{
    _coll_offs.push_back(_word_pos);
}

const UString& Corpus::get_sent(int s) const
{
    return _sents.at(s);
}

void Corpus::add_word(const UString word)
{
    int wid;
    if (_map_str2id.find(word) == _map_str2id.end())
    {
        wid = _map_id2str.size();
        _map_str2id[word] = wid;
        _map_id2str.push_back(word);
    }
    else
    {
        wid = _map_str2id[word];
    }
    _word_seqs.push_back(wid);
    ++_word_pos;
}

void Corpus::add_sent(const UString& sent)
{
    start_new_sent();
    _sents.push_back(sent);
}

void Corpus::add_doc(const UString& doc)
{
    start_new_doc();
}

}

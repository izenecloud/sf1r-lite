/*
 * corpus.h
 *
 *  Created on: 2010-10-18
 *      Author: jay
 */

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_

#include <iostream>
#include <vector>

namespace sf1r
{

class Corpus
{
public:
    Corpus()
        : _word_pos(0)
    {
    }

    inline int ntotal() const
    {
        return _word_seqs.size();
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

    void reset()
    {
        _word_pos = 0;
        _word_seqs.clear();
        _sent_offs.clear();
        _doc_offs.clear();
        _coll_offs.clear();
        _sents.clear();
        _coll_names.clear();
    }

    void start_new_sent()
    {
        _sent_offs.push_back(_word_pos);
    }

    void start_new_sent(const std::string& sent)
    {
        _sent_offs.push_back(_word_pos);
        _sents.push_back(sent);
    }

    void start_new_doc()
    {
        _doc_offs.push_back(_word_pos);
    }

    void start_new_coll()
    {
        _coll_offs.push_back(_word_pos);
    }

    void start_new_coll(const std::string& name)
    {
        _coll_offs.push_back(_word_pos);
        _coll_names.push_back(name);
    }

    void add_word(int wid)
    {
        _word_seqs.push_back(wid);
        ++_word_pos;
    }

    friend std::ostream& operator<<(std::ostream& out, const Corpus& corp)
    {
        out << "nDocs:" << corp.ndocs() << std::endl;
        out << "nTotal:" << corp.ntotal() << std::endl;
        out << "nSentences:" << corp.nsents() << std::endl;
//      out << "DocOffs:";
//      for(int i = 0; i < corp.m_aDocOffs.size(); ++ i)
//          out << corp.m_aDocOffs[i] << ",";
//      out << std::endl;
//      out << "SentOffs:";
//      for(int i = 0; i < corp.m_aSentOffs.size(); ++ i)
//          out << corp.m_aSentOffs[i] << ",";
//      out << std::endl;
//      out << "Words:";
//      for(int i = 0; i < corp.m_aWords.size(); ++ i)
//          out << i << ":" << IDManager::get_string(corp.m_aWords[i] ) << "(" << corp.m_aWords[i] << ")"<< ",";
//      out << std::endl;
        return out;
    }

    int* pack()
    {
        return 0;
    }

    void add_sent(const std::string& sent)
    {
        _sents.push_back(sent);
    }

    const std::string get_sent(int s) const
    {
        return _sents.at(s);
    }

    const std::string get_coll_name(int c) const
    {
        return _coll_names.at(c);
    }

private:
    int _word_pos;
    std::vector<int> _word_seqs;
    std::vector<int> _sent_offs;
    std::vector<int> _doc_offs;
    std::vector<int> _coll_offs;

    std::vector<std::string> _sents;
    std::vector<std::string> _coll_names;
};

}

#endif /* CORPUS_H_ */

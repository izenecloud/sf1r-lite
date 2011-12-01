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
        , _max_word_id(0)
    {
    }

    inline int ndocs() const
    {
        return _doc_offs.size();
    }

    inline int nwords() const
    {
        return _max_word_id;
    }

    inline int ntotal() const
    {
        return _word_seqs.size();
    }

    inline int nsents() const
    {
        return _sent_offs.size();
    }

    inline int ncolls() const
    {
        return _coll_offs.size();
    }

    inline int* get_doc_offs()
    {
        return &_doc_offs.at(0);
    }

    inline int* get_word_seq()
    {
        return &_word_seqs.at(0);
    }

    inline int* get_sent_offs()
    {
        return &_sent_offs.at(0);
    }

    inline int* get_coll_offs()
    {
        return &_coll_offs.at(0);
    }

    void init()
    {
        _word_seqs.clear();
        _doc_offs.clear();
        _sent_offs.clear();
        _word_pos = 0;
        _max_word_id = 0;
    }

    void start_new_doc()
    {
        _doc_offs.push_back(_word_pos);
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
        _max_word_id = std::max(wid, _max_word_id);
        _word_seqs.push_back(wid);
        ++_word_pos;
    }

    friend std::ostream& operator<<(std::ostream& out, const Corpus& corp)
    {
        out << "nDocs:" << corp.ndocs() << std::endl;
        out << "nWords:" << corp.nwords() << std::endl;
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
    int _max_word_id;
    std::vector<int> _word_seqs;
    std::vector<int> _doc_offs;
    std::vector<int> _sent_offs;

    std::vector<std::string> _sents;
    std::vector<int> _coll_offs;
    std::vector<std::string> _coll_names;

};

}

#endif /* CORPUS_H_ */

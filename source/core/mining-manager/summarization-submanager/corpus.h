/*
 * corpus.h
 *
 *  Created on: 2010-10-18
 *      Author: jay
 */

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_CORPUS_H_

#include "ts_common.h"

class Corpus
{
public:
    Corpus()
    {
        _word_pos = 0;
        _max_word_id = -1;
    }

    int ndocs()
    {
        return _doc_offs.size() - 1;
    }

    int nwords()
    {
        return _max_word_id + 1;
    }

    int ntotal()
    {
        return _word_seqs.size();
    }

    int nsents()
    {
        return _sent_offs.size() - 1;
    }

    int ncolls()
    {
        return _coll_offs.size() - 1;
    }


    int* get_doc_offs()
    {
        return &_doc_offs[0];
    }

    int* get_word_seq()
    {
        return &_word_seqs[0];
    }

    int* get_sent_offs()
    {
        return &_sent_offs[0];
    }

    int* get_coll_offs()
    {
        return &_coll_offs[0];
    }

    void init()
    {
        _word_seqs.clear();
        _doc_offs.clear();
        _sent_offs.clear();
        _word_pos = 0;
        _max_word_id = -1;
    }

    void start_new_doc()
    {
        _doc_offs.push_back(_word_pos);
    }

    void start_new_sent()
    {
        _sent_offs.push_back(_word_pos);
    }
    void start_new_sent(std::string& sent)
    {
        _sent_offs.push_back(_word_pos);
        _sents.push_back(sent);
    }
    void start_new_coll()
    {
        _coll_offs.push_back(_word_pos);
    }
    void start_new_coll(std::string& name)
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

    friend std::ostream& operator<<(std::ostream& out, Corpus& corp)
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

    void add_sent(std::string& sent)
    {
        _sents.push_back(sent);
    }

    std::string get_sent(int s)
    {
        return _sents[s];
    }

    std::string get_coll_name(int c)
    {
        return _coll_names[c];
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

#endif /* CORPUS_H_ */

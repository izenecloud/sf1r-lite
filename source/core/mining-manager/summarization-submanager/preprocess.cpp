#include "preprocess.h"
#include "util.h"
#include "sbd.h"
#include "stopword.h"

#include <util/hashFunction.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <dirent.h>

using namespace std;

namespace sf1r
{

void Preprocess::add_doc_to_corpus(const vector<string>& sentences,  Corpus& corpus)
{
    for (unsigned int i=0; i<sentences.size() ; i++)
    {
        string sent(sentences[i]);
        corpus.start_new_sent(sent);

        //Tokenize
        stringstream ss(sent);
        string token;

        while (getline(ss, token, ' '))
        {
            SPLMUtil::to_lowercase(token);
            if (Stopword::getInstance()->is_stop_word(token) || token.length() <= 1)
                continue;

            uint32_t wid = izenelib::util::izene_hashing(token);
            corpus.add_word(wid);
        }
    }
}

void Preprocess::runPreprocess(const string& corpus_root, const string& stop_word_file, Corpus& corpus)
{
    Stopword::getInstance()->load(stop_word_file);

    DIR             *dip;
    DIR             *diSub;
    struct dirent   *dit;

    if ((dip = opendir((const char *)corpus_root.c_str())) == NULL)
    {
        perror("opendir");
        return;
    }

    string collName = "Summary";
    corpus.start_new_coll(collName);
    corpus.start_new_doc();

    while ((dit = readdir(dip)) != NULL)
    {
        if ((diSub = opendir(dit->d_name)) == NULL &&
                dit->d_type == 0x8)
        {
            string STRING;
            string dirStr = corpus_root + "/";
            string fileName = dirStr + dit->d_name;

            ifstream infile;
            infile.open (fileName.c_str());
            while (!infile.eof())
            {
                getline(infile,STRING);
                vector<string> sentences;
                SentenceBoundaryDetection::sbd(STRING, sentences);
                add_doc_to_corpus(sentences, corpus);
            }
            infile.close();
        }
    }

    corpus.start_new_doc();
    corpus.start_new_sent();
    corpus.start_new_coll();

    if (closedir(dip) == -1)
    {
        perror("closedir");
        return;
    }
}

}

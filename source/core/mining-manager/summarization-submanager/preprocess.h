///
/// @file preprocess.h
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_PREPROCESS_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_PREPROCESS_H_

#include "corpus.h"

#include <string>
#include <vector>

namespace sf1r
{

class Preprocess
{
public:
    static void add_doc_to_corpus(const std::vector<std::string>& sentences, Corpus& corpus);

    static void runPreprocess(const std::string& corpus_root, const std::string& stop_word_file, Corpus& corpus);
};

}

#endif

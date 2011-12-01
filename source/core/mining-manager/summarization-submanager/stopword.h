///
/// @file stopword.h
/// @author jay
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_STOPWORD_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_STOPWORD_H_

#include "ts_common.h"

class Stopword
{
private:
    typedef __gnu_cxx::hash_set<std::string> HashSet;
    static HashSet m_oStopwordSet;
    static Stopword * INSTANCE;
    Stopword() {}

public:
    static Stopword * getInstance()
    {
        if (!INSTANCE)
        {
            INSTANCE = new Stopword();
        }
        return INSTANCE;
    }

    static void load(const std::string& file = "../resource/stopwords.txt");
    static bool is_stop_word(std::string& word);
};

#endif /* STOPWORD_H_ */

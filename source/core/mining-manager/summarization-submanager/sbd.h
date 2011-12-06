///
/// @file sbd.h
/// @author Hogyeong Jeong ( hogyeong.jeong@gmail.com )
///

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SBD_H_
#define SF1R_MINING_MANAGER_SUMMARIZATION_SBD_H_

#include <cstring>
#include <sstream>

class SentenceBoundaryDetection
{
public:
    static bool startsWithLowercase(const std::string& str)
    {
        return !str.empty() && str[0] >= 'a' && str[0] <= 'z';
    }

    static bool endsWith(const std::string& str, const std::string& substr)
    {
        if (str.length() < substr.length()) return false;
        size_t pos = str.length() - 1;
        size_t subpos = substr.length() - 1;
        for (; subpos >= 0; --pos, --subpos)
        {
            if (str[pos] != substr[subpos])
                return false;
        }
        return true;
    }

    static void sbd(const std::string& text, std::vector<std::string>& sentences)
    {
        std::string currentString;
        std::stringstream sss(text);
        std::string stc;
        while (getline(sss, stc, '\n'))
        {
            std::stringstream ss(stc);

            getline(ss, currentString, ' ');
            std::string nextString;
            std::string sentence;
            while (getline(ss, nextString, ' '))
            {
                if (endsWith(currentString, "!") || endsWith(currentString, "?"))
                {
                    if (!sentence.empty())
                        sentence.push_back(' ');
                    sentence.append(currentString);
                    sentences.push_back(std::string());
                    sentence.swap(sentences.back());
                }
                else if (endsWith(currentString, ".") && !(endsWith(nextString, ".")
                            && nextString.size() == 2)  && !startsWithLowercase(nextString))
                {
                    if (!sentence.empty())
                        sentence.push_back(' ');
                    sentence.append(currentString);
                    sentences.push_back(std::string());
                    sentence.swap(sentences.back());
                }
                else
                {
                    if (!sentence.empty())
                        sentence.push_back(' ');
                    sentence.append(currentString);
                }
                currentString.assign(nextString);
            }
            if (!sentence.empty())
                sentence.push_back(' ');
            sentence.append(currentString);
            if (!sentence.empty())
                sentences.push_back(sentence);

        }
    }

};

#endif

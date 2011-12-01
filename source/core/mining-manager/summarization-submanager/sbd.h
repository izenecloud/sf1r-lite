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
    static bool startsWithLowercase(std::string str)
    {
        return !str.empty() && str[0] >= 'a' && str[0] <= 'z';
    }

    static bool endsWith(std::string str, std::string substr)
    {
        size_t i = str.rfind(substr);
        return i != std::string::npos && i == str.length() - substr.length();
    }

    static std::vector<std::string> sbd(std::string text)
    {
        std::string currentString;
        std::stringstream sss(text);
        std::string stc;
        std::vector<std::string> sentences;
        while (getline(sss, stc, '\n'))
        {
            std::stringstream ss(stc);

            std::getline(ss, currentString, ' ');
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
        return sentences;
    }

};

#endif

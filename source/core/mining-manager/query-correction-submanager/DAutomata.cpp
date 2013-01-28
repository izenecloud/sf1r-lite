/**
* @brief  source file of class DAutomata, it is used to construct dictionary automata according
* to given dictionary,currently we use support two types of language:english and korean
* @author Jinli Liu
* @date Sep 15,2009
* @version v2.0
* @detail
* -Log
* --2009.11.24 combine buildTrieEN and buildTrieKR to one function buildTrie()
*/
#include "DAutomata.h"
namespace sf1r
{

//Builds a trie from given dictionary, note dictionary only with one word in each line
DictState* DAutomata::buildTrie(string dictFileName, vector<mychar>& phonemes,
                                rde_map& alphabet, rde_hash& lexicon_)
{
    phonemes.clear();
    alphabet.clear();
    lexicon_.clear();
    DictState *start = new DictState;
    std::string w;
    int num = 0;
    ifstream input(dictFileName.c_str());
    if ( !input.is_open() )
    {
        cout << "Error : File(" << dictFileName << ") is not opened." << endl;
        return start;
    }
    while (input.good())
    {
        getline(input, w);
        if (w.find("\r") != std::string::npos)
            w = w.substr(0, w.length()-1);

        izenelib::util::UString srcWord(w, izenelib::util::UString::UTF_8);
        srcWord.toLowerString();
        lexicon_.insert(srcWord, ' ');
        UString ustrWord;
        izenelib::util::ustring_tool::processKoreanDecomposerWithCharacterCheck(
            srcWord, ustrWord);
        DictState *s = start;
        unsigned int p = 0;
        while (p<ustrWord.length() && s->next(ustrWord[p])!=NULL)
        {
            s = s->next(ustrWord[p]);
            p++;
        }
        DictState *s1;
        while (p < ustrWord.length())
        {
            s->setNext(ustrWord[p], (s1 = new DictState));
            p++;
            s = s1;
        }
        s->setFinal();
        for (int i=0; i<(int)ustrWord.length(); i++)
        {
            if (alphabet.find(ustrWord[i])!=alphabet.end())
                continue;
            else
                alphabet.insert(rde::pair<mychar, int>(ustrWord[i], num++));
        }
    }
    input.close();
    phonemes.resize(alphabet.size(), 0);
    for (rde::hash_map<mychar,int>::iterator it = alphabet.begin();it!=alphabet.end();it++)
        phonemes[it->second] = it->first;
    return start;
}//end of buildTrie


int DictState::TOTAL_STATES = 0;
}


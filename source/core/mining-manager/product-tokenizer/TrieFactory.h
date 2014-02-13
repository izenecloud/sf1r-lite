/**
 * @file TrieFactory.h
 * @brief create Trie instance.
 */

#ifndef TRIE_FACTORY_H
#define TRIE_FACTORY_H

#include <am/succinct/ux-trie/uxTrie.hpp>
#include <util/singleton.h>

namespace sf1r
{

class TrieFactory
{
public:
    ~TrieFactory()
    {
        std::map<std::string, izenelib::am::succinct::ux::Trie*>::iterator tit = tries_.begin();
        for (; tit != tries_.end(); ++tit)
            delete tit->second;
    }

    static TrieFactory* Get()
    {
        return izenelib::util::Singleton<TrieFactory>::get();
    }

    izenelib::am::succinct::ux::Trie* GetTrie(const std::string& dictPath)
    {
        std::map<std::string, izenelib::am::succinct::ux::Trie*>::iterator kit = tries_.find(dictPath);
        if (kit != tries_.end())
            return kit->second;

        izenelib::am::succinct::ux::Trie* trie = new izenelib::am::succinct::ux::Trie;
        std::vector<std::string> keyList;
        ReadKeyList_(dictPath, keyList);
        trie->build(keyList);
        tries_[dictPath] = trie;
        return trie;
    }

private:
    int ReadKeyList_(
        const std::string& fn,
        std::vector<std::string>& keyList)
    {
        std::ifstream ifs(fn.c_str());
        if (!ifs)
        {
            std::cerr << "cannot open " << fn << std::endl;
            return -1;
        }

        for (std::string key; getline(ifs, key);)
        {
            if (!key.empty() && key[key.size() - 1] == '\r')
                keyList.push_back(key.substr(0, key.size() - 1));
            else
                keyList.push_back(key);
        }
        return 0;
    }

private:
    std::map<std::string, izenelib::am::succinct::ux::Trie*> tries_;
};

}

#endif // TRIE_FACTORY_H

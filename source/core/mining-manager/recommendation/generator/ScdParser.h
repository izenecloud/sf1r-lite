#ifndef SCD_PARSER_H
#define SCD_PAESER_H

#include <string>
#include <iostream>
#include <fstream>

namespace sf1r
{
namespace Recommend
{
class ScdParser
{
public:
    class ScdDoc
    {
    public:
        ScdDoc()
            : title_("")
            , category_("")
        {
        }
        ScdDoc(const std::string& title,
               const std::string& category)
            : title_(title)
            , category_(category)
        {
        }
    public:
        void reset()
        {
            title_ = "";
            category_ = "";
        }

        const std::string& title() const
        {
            return title_;
        }

        void setTitle(const std::string& ti)
        {
            title_ = ti;
        }

        const std::string& category() const
        {
            return category_;
        }

        void setCategory(const std::string& cat)
        {
            category_ = cat;
        }
    private:
        std::string title_;
        std::string category_;
    };
    
    class iterator
    {
    public:
        iterator(ScdParser* sp)
            : sp_(sp)
            , nDoc_(-1)
        {
        }
    public:
        static void reset()
        {
            //sp_ = sp;
            iterator::NUM_GENERATOR = 1;
        }
        
        iterator& operator++();
        //iterator& operator++(int);
        bool operator== (const iterator& other) const;
        bool operator!= (const iterator& other) const;
        iterator& operator=(const iterator& other);
        const ScdDoc& operator*() const;
        const ScdDoc* operator->() const; 
    private:
        ScdParser* sp_;
        int nDoc_;
        static int NUM_GENERATOR;

    friend class ScdParser;
    };
    
    
public:
    ScdParser();
    ~ScdParser();
public:
    void load(const std::string& path);
    void reset();

    iterator& begin();
    iterator& end();
    
    static bool isValid(const std::string& path);
    
private:
    bool parseDoc();
private:
    std::ifstream in_;
    ScdParser::ScdDoc* doc_;
    ScdParser::iterator* it_;

friend class ScdParser::iterator;
};
}
}
#endif

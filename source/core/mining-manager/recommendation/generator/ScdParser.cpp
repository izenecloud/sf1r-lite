#include "ScdParser.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    

#include <iostream>
#include <fstream>

#include <string.h>

namespace sf1r
{
namespace Recommend
{

ScdParser::ScdParser()
{
    it_ = new ScdParser::iterator(this);
    doc_ = new ScdParser::ScdDoc();
}

ScdParser::~ScdParser()
{
    if (NULL != it_)
    {
        delete it_;
        it_ = NULL;
    }
    if (NULL != doc_)
    {
        delete doc_;
        doc_ = NULL;
    }
}

void ScdParser::load(const std::string& path)
{
    if (!isValid(path))
    {
    }
    reset();
    in_.open(path.c_str(), std::ifstream::in);
}

void ScdParser::reset()
{
    if (in_.is_open())
        in_.close();
    ScdParser::iterator::reset();
    doc_->reset();
}

bool ScdParser::isValid(const std::string& path)
{
    if (!boost::filesystem::exists(path))
        return false;
    std::string fileName = boost::filesystem::path(path).stem().string();
    boost::to_lower(fileName);
    if (fileName[24] == 'u')
        return true;
    return false;
}

int ScdParser::iterator::NUM_GENERATOR = 0;

ScdParser::iterator& ScdParser::begin()
{
     bool ret = parseDoc();
     if (ret)
     {
        it_->nDoc_ = ScdParser::iterator::NUM_GENERATOR;
        ScdParser::iterator::NUM_GENERATOR++;
     }
     else
     {
        it_->nDoc_ = -1;
     }
    return *it_;
}

ScdParser::iterator& ScdParser::end()
{
    static ScdParser::iterator it(this);
    it.nDoc_ = -1;
    return it;
}

bool ScdParser::parseDoc()
{
    doc_->reset();
    static bool docIdReadedLastTime = false;
    bool hasNextDoc = false;
    
    static const uint32_t LINE_SIZE = 102400;
    bool ret = false;
    static char cLine[LINE_SIZE];
    memset(cLine, 0, LINE_SIZE);
    while(in_.getline(cLine, LINE_SIZE))
    {
        if ("<DOCID>" == std::string(cLine, strlen("<DOCID>")))
        {
            if (docIdReadedLastTime)
            {
                hasNextDoc = true;
                break;
            }
            else
                docIdReadedLastTime = true;
        }
        if ("<Title>" == std::string(cLine, strlen("<Title>")))
        {
            doc_->setTitle(std::string(cLine + strlen("<Title>")));
            ret = true;
        }
        else if ("<Category>" == std::string(cLine, strlen("<Category>")))
        {
            doc_->setCategory(std::string(cLine + strlen("<Category>")));
            ret = true;
        }
    }
    docIdReadedLastTime = hasNextDoc;
    return ret | hasNextDoc;
}

ScdParser::iterator& ScdParser::iterator::operator++()
{
    bool ret = sp_->parseDoc();
    if (ret)
    {
        nDoc_ = ScdParser::iterator::NUM_GENERATOR;
        ScdParser::iterator::NUM_GENERATOR++;
    }
    else
    {
        nDoc_ = -1;
    }
    return *this;
}

const ScdParser::ScdDoc& ScdParser::iterator::operator*() const
{
    return *(sp_->doc_);
}

const ScdParser::ScdDoc* ScdParser::iterator::operator->() const
{
    return (sp_->doc_);
}

bool ScdParser::iterator::operator== (const iterator& other) const
{
    return nDoc_ == other.nDoc_;
}

bool ScdParser::iterator::operator!= (const iterator& other) const
{
    return nDoc_ != other.nDoc_;
}

ScdParser::iterator& ScdParser::iterator::operator=(const iterator& other)
{
    sp_ = other.sp_;
    nDoc_ = other.nDoc_;
    return *this;
}

}
}

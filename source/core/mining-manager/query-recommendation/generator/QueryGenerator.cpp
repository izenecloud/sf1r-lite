#include "QueryGenerator.h"
#include "ScdParser.h"
#include <boost/filesystem.hpp>
#include <knlp/normalize.h>
#include <fstream>

namespace sf1r
{
namespace Recommend
{
QueryGenerator::QueryGenerator(const std::string& scdDir)
    : scdDir_(scdDir)
    , tokenizer_(NULL)
{
    open_();
}

void QueryGenerator::open_()
{
    try
    {
        if (!boost::filesystem::exists(scdDir_))
            return;
        if (boost::filesystem::is_regular_file(scdDir_))
            scdFiles_.push_back(scdDir_);
        else if(boost::filesystem::is_directory(scdDir_))
        {
            boost::filesystem::directory_iterator end;
            for(boost::filesystem::directory_iterator it(scdDir_) ; it != end ; ++it)
            {
                const std::string p = it->path().string();
                
                if(boost::filesystem::is_regular_file(p))
                {
                    if (ScdParser::isValid(p))
                        scdFiles_.push_back(p);
                }
            }
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        std::cout << ex.what() << '\n';
    }

}

void QueryGenerator::generate()
{
    if (NULL == tokenizer_)
        return;
    ScdParser* sp = new ScdParser();
    for (std::size_t i = 0; i < scdFiles_.size(); i++)
    {
        std::ofstream out;
        out.open((scdFiles_[i] + ".terms").c_str(), std::ofstream::out | std::ofstream::trunc);
        sp->load(scdFiles_[i]);
        ScdParser::iterator it= sp->begin();
        for(; it != sp->end(); ++it)
        {
            std::string title = it->title();
            ilplib::knlp::Normalize::normalize(title);
            Tokenize::TermVector terms;
            (*tokenizer_)(title, terms);
            for (std::size_t i = 0; i < terms.size(); i++)
            {
                out<<terms[i].term()<<" ";
            }
            out<<"\n";
            //std::cout<<title<<" "<<it->category()<<"\n";
        }
        out.close();
    }
    delete sp;
}

}
}

#include "LAConfigUnit.h"

#include <iostream> //TEMP

using namespace std;


namespace sf1r
{

LAConfigUnit::LAConfigUnit()
{
    clear();
}

LAConfigUnit::LAConfigUnit( const string& id, const string& analysis )
{
    clear();
    id_ = id;
    analysis_ = analysis;
}

/**
 * @brief
 * @param analysis
 */
void LAConfigUnit::addReferenceMethod( const LAConfigUnit & method )
{
    langRefMap_.insert( pair<string, string>(method.getLanguage(), method.getId()) );
}

/**
 * @brief
 *
 * @return
 */
bool LAConfigUnit::getMethodIdByLang( const std::string & language, std::string & method_id  ) const
{
    std::map<std::string, std::string>::const_iterator it;

    if ( (it = langRefMap_.find( language )) == langRefMap_.end() )
    {
        return false;
    }
    else
    {
        method_id = it->second;
        return true;
    }
}

void LAConfigUnit::clear()
{
    id_ = "";
    analysis_ = "";
    langRefMap_.clear();
    level_ = 0;
    apart_ = false;
    minLevel_ = 2;
    maxLevel_ = 2;
    maxNo_ = 0;
    language_ = "";
    mode_ = "";
    option_ = "";
    specialChar_ = "";
    dictionaryPath_ = "";
    prefix_ = false;
    suffix_ = false;
    caseSensitive_ = false;
    idxFlag_ = (unsigned char)0x77;
    schFlag_ = (unsigned char)0x77;
    stem_ = true;
    lower_ = true;
}

std::ostream & operator<<( std::ostream & out, const LAConfigUnit & unit )
{
    out << "<id=" << unit.id_ << "> <analysis=" << unit.analysis_ << ">";
    out << "(caseSensitive=[" << unit.caseSensitive_ << "] idxFlag=[" <<
    (unsigned int)unit.idxFlag_ << "] schFlag=[" << (unsigned int)unit.schFlag_ << "])";

    if ( (unit.analysis_).compare( "ngram" ) == 0 )
    {
        out << " (minleve=[" << unit.minLevel_ << "] maxlevel=[" << unit.maxLevel_ << "] maxno=["
        << unit.maxNo_ << "] apart=[" << unit.apart_ << "])";
    }
    else if ( (unit.analysis_).compare( "matrix" ) == 0 )
    {
        out << " (prefix=[" << unit.prefix_ << "] suffix=[" << unit.suffix_ << "])";
    }
    else if ( (unit.analysis_).compare( "korean" ) == 0
              || (unit.analysis_).compare( "english" ) == 0
              || (unit.analysis_).compare( "chinese" ) == 0)
    {
        out << " (mode=[" << unit.mode_
        << "] option=[" << unit.option_ << "] specialChar=[" << unit.specialChar_
        << "] dictionarypath=[" << unit.dictionaryPath_ << "])";
    }
    else if ( (unit.analysis_).compare( "multilang" ) == 0 )
    {
        out << " (stem=[" << unit.stem_ << "] lower=[" << unit.lower_ <<
        "] advoption=[" << unit.advOption_ << "])";
    }
    else
    {
        out << " (minleve=[" << unit.minLevel_ << "] maxlevel=[" << unit.maxLevel_ << "] maxno=["
        << "] prefix=[" << unit.prefix_ << "] suffix=[" << unit.suffix_
        << "] mode=[" << unit.mode_
        << "] option=[" << unit.option_ << "] specialChar=[" << unit.specialChar_
        << "] dictionarypath=[" << unit.dictionaryPath_ << "])";
    }
    /*
    else ( (unit.analysis_).compare( "auto" ) == 0  )
    {
        out << " @(refs=";

        std::map<std::string, std::string>::const_iterator it;
        for( it = (unit.langRefMap_).begin(); it != (unit.langRefMap_).end(); it++ )
        {
            out << it->first << "|" << it->second << ", ";
        }
        out << "\")";
    }
    */
    return out;
}


} // namespace


// eof

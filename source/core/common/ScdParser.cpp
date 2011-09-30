#include "ScdParser.h"
#include <iostream>
#include <cassert>
#include <cstdio>

#include <util/ustring/UString.h>
#include <util/profiler/ProfilerGroup.h>

#include <boost/filesystem/path.hpp>
#define BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_while.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>
#include <iterator>
#include <boost/regex.hpp>

using namespace boost::filesystem;

using namespace BOOST_SPIRIT_CLASSIC_NS;

using namespace std;

namespace
{
/** default delimiter for each doc */
const char* DEFAULT_DOC_DELIMITER = "<DOCID>";

/** pattern for html codes of "<>" */
const boost::regex PATTERN_LT_GT("(&lt;)|(&gt;)");

/** pattern for end of line */
const boost::regex PATTERN_EOL("\n|\r\n|\r|$");

/** format string for "<>" */
const char* FORMAT_LT_GT = "(?1<)(?2>)";
}

struct scd_grammar
            : public grammar<scd_grammar>
{
    SCDDoc& scddoc;
    izenelib::util::UString::EncodingType& codingType;
    izenelib::util::UString property_name;

    scd_grammar(SCDDoc& doc, izenelib::util::UString::EncodingType& type):scddoc(doc), codingType(type) {}

    struct print
    {
        void operator()(const char *begin, const char *end) const
        {
            std::cout << std::string(begin, end) << std::endl;
        }
    };

    struct add_property_name
    {
        add_property_name(SCDDoc& doc, izenelib::util::UString::EncodingType& type, izenelib::util::UString& propertyname)
                :scddoc(doc),codingType(type),property_name(propertyname) {}
        void operator()(const char *begin, const char *end) const
        {
            property_name.assign(std::string(begin,end),codingType);
        }
        SCDDoc& scddoc;
        izenelib::util::UString::EncodingType& codingType;
        izenelib::util::UString& property_name;
    };

    struct add_property_value
    {
        add_property_value(SCDDoc& doc, izenelib::util::UString::EncodingType& type, izenelib::util::UString& propertyname)
                :scddoc(doc),codingType(type),property_name(propertyname) {}
        void operator()(const char *begin, const char *end) const
        {
            // in ScdParser::iterator::preProcessDoc(), "<" is replaced by "&lt;", and ">" by "&gt;",
            // as ">" is used to split group path values, they are converted back here
            std::string str;
            boost::regex_replace(std::back_inserter(str), begin, end,
                PATTERN_LT_GT, FORMAT_LT_GT, boost::match_default | boost::format_all);

            izenelib::util::UString property_value(str,codingType);
            scddoc.push_back(FieldPair(property_name, property_value));
            property_name.clear();
        }
        SCDDoc& scddoc;
        izenelib::util::UString::EncodingType& codingType;
        izenelib::util::UString& property_name;
    };

    template <typename Scanner>
    struct definition
    {
        rule<Scanner> object, member, docid, propertypairs, propertypair, propertyName, propertyValue;

        definition(const scd_grammar &self)
        {
            using namespace boost::spirit;

            object = *(propertypair);
            propertypair = ch_p('<') >>
                           propertyName[add_property_name(self.scddoc,self.codingType,const_cast<izenelib::util::UString&>(self.property_name))]
                           >> ch_p('>') >>
                           propertyValue[add_property_value(self.scddoc,self.codingType,const_cast<izenelib::util::UString&>(self.property_name))];

            // WARN: This rule should also apply to field name on XmlConfigParser.cpp.
            // Those two should share same field name rule to make easy validating.

            propertyName = *(alnum_p|ch_p('-')|ch_p('_'));
            propertyValue = *(~(ch_p('<')));
        }

        const rule<Scanner> &start()
        {
            return object;
        }
    };
};

ScdParser::ScdParser()
        :size_(0), encodingType_(izenelib::util::UString::UTF_8), docDelimiter_(DEFAULT_DOC_DELIMITER)
{}

ScdParser::ScdParser( const izenelib::util::UString::EncodingType & encodingType )
        :size_(0), encodingType_(encodingType), docDelimiter_(DEFAULT_DOC_DELIMITER)
{}

ScdParser::ScdParser(const izenelib::util::UString::EncodingType & encodingType, const char* docDelimiter)
        :size_(0), encodingType_(encodingType), docDelimiter_(docDelimiter)
{}

ScdParser::~ScdParser()
{
    if (fs_.is_open())
        fs_.close();
}

bool ScdParser::checkSCDFormat( const string & file )
{
    //B-<document collector ID(2Bytes)>-<YYYYMMDDHHmm(12Bytes)>-
    //    <SSsss(second, millisecond, 5Bytes)>-<I/U/D/R(1Byte)>-
    //    <C/F(1Byte)>.SCD

    if ( boost::filesystem::path(file).extension() != ".SCD"
            && boost::filesystem::path(file).extension() != ".scd"  )
        return false;

    string fileName = boost::filesystem::path(file).stem();

    size_t pos = 0;
    string strVal;
    int val;

    if ( fileName.length() != 27 )
        return false;
    if ( fileName[pos] != 'B' && fileName[0] != 'b' )
        return false;
    if ( fileName[++pos] != '-' )
        return false;

    pos++;
    strVal = fileName.substr( pos, 2 );        //collectorID
    sscanf( strVal.c_str(), "%d", &val );

    pos += 3;
    strVal = fileName.substr( pos, 4 );        //year
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 1970 )
        return false;

    pos += 4;
    strVal = fileName.substr( pos, 2 );        //month
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 1 || val > 12 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //day
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 1 || val > 31 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //hour
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 0 || val > 24 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 2 );        //minute
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 0 || val >= 60 )
        return false;

    pos += 3;
    strVal = fileName.substr( pos, 2 );        //second
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 0 || val >= 60 )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 3 );        //millisecond
    sscanf( strVal.c_str(), "%d", &val );
    if ( val < 0 || val > 999 )
        return false;

    pos += 4;
    strVal = fileName.substr( pos, 1 );        //c
    char c = tolower( strVal[0] );
    if ( c != 'i' && c != 'd' && c != 'u' && c != 'r' )
        return false;

    pos += 2;
    strVal = fileName.substr( pos, 1 );        //d
    c = tolower( strVal[0] );
    if ( c != 'c' && c != 'f' )
        return false;


    return true;
}

unsigned ScdParser::checkSCDDate( const string& file)
{
    static const unsigned kOffset = 5; // strlen("B-01-")
    static const unsigned kCount = 8; // strlen("YYYYMMDD")

    try
    {
        string strVal = boost::filesystem::path(file).stem().substr(kOffset, kCount);
        unsigned val = 0;
        sscanf( strVal.c_str(), "%u", &val );
        return val;
    }
    catch (const std::out_of_range&)
    {
        return 0U;
    }
}

unsigned ScdParser::checkSCDTime( const string& file)
{
    static const unsigned kOffset = 13; // strlen("B-01-YYYYMMDD")
    static const unsigned kCount = 10; // strlen("HHmm-SSssss")

    try
    {
        string strVal = boost::filesystem::path(file).stem().substr(kOffset, kCount);
        unsigned hhmm = 0;
        unsigned sssss = 0;
        sscanf( strVal.c_str(), "%u-%u", &hhmm, &sssss );
        return hhmm * 100000 + sssss;
    }
    catch (const std::out_of_range&)
    {
        return 0U;
    }
}

SCD_TYPE ScdParser::checkSCDType( const string& file )
{
    //B-<document collector ID(2Bytes)>-<YYYYMMDDHHmm(12Bytes)>-
    //    <SSsss(second, millisecond, 5Bytes)>-<I/U/D/R(1Byte)>-
    //    <C/F(1Byte)>.SCD

    string fileName = boost::filesystem::path(file).stem();

    SCD_TYPE type = NOT_SCD;

    // strlen("B-01-YYYYMMDDHHmm-SSsss-")
    const static unsigned kTypeOffset = 24;

    if (fileName.size() > kTypeOffset)
    {
        switch ( tolower(fileName[kTypeOffset]) )
        {
        case 'i':
            type = INSERT_SCD;
            break;
        case 'd':
            type = DELETE_SCD;
            break;
        case 'u':
            type = UPDATE_SCD;
            break;
        default:
            type = NOT_SCD;
            break;
        }
    }

    return type;
}

bool ScdParser::compareSCD (const string & file1, const string & file2)
{
    if (checkSCDType(file1) == checkSCDType(file2) )
    {
        if (checkSCDDate(file1) == checkSCDDate(file2))
            return (checkSCDTime(file1) < checkSCDTime(file2));
        return (checkSCDDate(file1) < checkSCDDate(file2));
    }

    return (checkSCDType(file1) < checkSCDType(file2));
}


bool ScdParser::load(const string& path)
{
    if (fs_.is_open())
    {
        fs_.close();
    }
    fs_.open(path.c_str(), ios::in);
    if (!fs_)
        return false;
    fs_.seekg(0, ios::end);
    size_ = fs_.tellg();
    fs_.seekg(0, ios::beg);
    return true;
}

bool ScdParser::getDocIdList( std::vector<izenelib::util::UString> & list )
{
    if ( !fs_.is_open() )
        return false;

    ScdParser::iterator it = this->begin();
    for ( ; it != this->end(); it++ )
    {
        //if the document is NULL
        if ( (*it) == NULL  )
        {
            return false;
        }

        list.push_back( (*it)->at(0).second );
        docOffsetList_.insert( (*it)->at(0).second, it.getOffset() );
    }

    return true;
}

bool ScdParser::getDocIdList( std::vector<DocIdPair > & list )
{
    if ( !fs_.is_open() )
        return false;

    ScdParser::iterator it = this->begin();

    for ( ; it != this->end(); it++ )
    {
        //if the document is NULL
        if ( (*it) == NULL  )
        {
            return false;
        }
        list.push_back( make_pair((*it)->at(0).second, 0 ));
        docOffsetList_.insert( (*it)->at(0).second, it.getOffset() );
    }

    return true;
}

ScdParser::iterator ScdParser::begin(unsigned int start_doc)
{
    fs_.clear();
    fs_.seekg(0, ios::beg);
    return iterator(this, start_doc);
}

ScdParser::iterator ScdParser::begin(const std::vector<string>& propertyNameList, unsigned int start_doc)
{
    fs_.clear();
    fs_.seekg(0, ios::beg);
    return iterator(this, start_doc, propertyNameList);
}

ScdParser::iterator ScdParser::end()
{
    //return iterator(size_);
    return iterator(-1);
}

bool ScdParser::getDoc( const izenelib::util::UString & docId, SCDDoc& doc )
{
    long * val = docOffsetList_.find( docId );
    if ( val == NULL )
    {
        return false;
    }

    if ( fs_.eof() )
    {
        fs_.clear();
    }
    fs_.seekg( *val, ios::beg );

    iterator it( this, 0 );
    doc = *(*it);

    return true;
}

ScdParser::iterator::iterator(long offset)
        :pfs_(NULL)
        ,readLength_(0)
        ,docDelimiter_(DEFAULT_DOC_DELIMITER)
{
    offset_ = offset;
}

ScdParser::iterator::iterator(ScdParser* pScdParser, unsigned int start_doc)
        :docDelimiter_(pScdParser->docDelimiter_)
{
    pfs_ = &(pScdParser->fs_);
    pfs_->clear();
    readLength_ = 0;
    codingType_ = pScdParser->getEncodingType();
    offset_ = 0;
    buffer_.reset(new izenelib::util::izene_streambuf);
    readLength_ = izenelib::util::izene_read_until(*pfs_,*buffer_,docDelimiter_);
    if (readLength_ > 0)
    {
        buffer_->consume(readLength_);
        doc_.reset(getDoc());
        if (start_doc > 0)
        {
            for (unsigned int i = 0; i < start_doc; ++i)
                operator++();
        }
    }
    else
    {
        offset_ = prevOffset_ = -1;
    }

}

ScdParser::iterator::iterator(ScdParser* pScdParser, unsigned int start_doc, const std::vector<string>& propertyNameList)
    :docDelimiter_(pScdParser->docDelimiter_)
    ,propertyNameList_(propertyNameList)
{
    pfs_ = &(pScdParser->fs_);
    pfs_->clear();
    readLength_ = 0;
    codingType_ = pScdParser->getEncodingType();
    offset_ = 0;
    buffer_.reset(new izenelib::util::izene_streambuf);
    readLength_ = izenelib::util::izene_read_until(*pfs_,*buffer_,docDelimiter_);
    if(readLength_ > 0)
    {
        buffer_->consume(readLength_);
        doc_.reset(getDoc());
        if(start_doc > 0)
        {
            for(unsigned int i = 0; i < start_doc; ++i)
                operator++();
        }
    }
    else
    {
        offset_ = prevOffset_ = -1;
    }
}

ScdParser::iterator::iterator(const iterator& other)
        :pfs_(other.pfs_)
        ,readLength_(0)
        ,prevOffset_(other.prevOffset_)
        ,offset_(other.offset_)
        ,doc_(other.doc_)
        ,codingType_(other.codingType_)
        ,buffer_(other.buffer_)
        ,docDelimiter_(other.docDelimiter_)
{
}

ScdParser::iterator::~iterator()
{
}

ScdParser::iterator& ScdParser::iterator::operator=(const iterator& other)
{
    pfs_ = other.pfs_;
    readLength_ = 0;
    prevOffset_ =other.prevOffset_;
    offset_ = other.offset_;
    doc_ = other.doc_;
    codingType_ = other.codingType_;
    buffer_	 = other.buffer_;
    docDelimiter_ = other.docDelimiter_;
    return *this;
}

bool ScdParser::iterator::operator==(const iterator& other)const
{
    return offset_ == other.offset_;
}

bool ScdParser::iterator::operator!=(const iterator& other)const
{
    return (offset_ != other.offset_);
}

void ScdParser::iterator::operator++()
{
    if ( pfs_->eof() )
    {
        offset_ = -1;
    }
    else
    {
        offset_ = prevOffset_;
        doc_.reset(getDoc());
    }
    //return iterator(*this);
}

void ScdParser::iterator::operator++(int)
{
    if ( pfs_->eof() )
    {
        offset_ = -1;
    }
    else
    {
        offset_ = prevOffset_;
        doc_.reset(getDoc());
    }
    //return iterator(*this);
}

SCDDocPtr ScdParser::iterator::operator*()
{
    return doc_;
}

long ScdParser::iterator::getOffset()
{
    return offset_;
}

SCDDoc* ScdParser::iterator::getDoc()
{
    CREATE_PROFILER ( proScdParsing, "Index:SIAProcess", "Scd Parsing : parse SCD file");

    START_PROFILER ( proScdParsing );
    readLength_ = izenelib::util::izene_read_until(*pfs_,*buffer_,docDelimiter_);
    string str(docDelimiter_);
    if (readLength_ > 0)
    {
        str.append(std::string(buffer_->gptr(),readLength_-7));
        buffer_->consume(readLength_);
        prevOffset_ = pfs_->tellg();
    }
    else
    {
        do
        {
            readLength_ = izenelib::util::izene_read_until(*pfs_,*buffer_,PATTERN_EOL);
            if (readLength_ > 0)
            {
                str.append(std::string(buffer_->gptr(),readLength_));
            }
            buffer_->consume(readLength_);
        }
        while (readLength_ != 0);
        prevOffset_  = -1;
    }
    SCDDoc* doc = new SCDDoc;
    if (str == docDelimiter_)
        return doc;

    /// It's recommended to handle this processing in application by which SCD is created.
    preProcessDoc(str);

    scd_grammar g(*doc, codingType_);
    parse_info<> pi = parse(str.c_str(), g, space_p);
    STOP_PROFILER ( proScdParsing );

    return doc;
}

void ScdParser::iterator::preProcessDoc(string& strDoc)
{
    if (propertyNameList_.empty())
        return;

    string tmpStr;
    size_t docLen = strDoc.size();
    size_t tagLen, tagEnd;
    bool matchName;
    char ch;
    for (size_t i = 0; i < docLen; )
    {
        ch = strDoc[i];

        if (ch == '<')
        {
            // match property name between '<' and '>'
            matchName = false;
            for (size_t t = 0; t < propertyNameList_.size(); t ++)
            {
                tagLen = propertyNameList_[t].size();
                tagEnd = i+tagLen+1;

                if ( tagEnd < docLen && strDoc[tagEnd] == '>'
                        && boost::iequals(string(strDoc, i+1, tagLen), propertyNameList_[t])) // case insensitively
                {
                    matchName = true;
                    // keep property name tag
                    while (i < tagEnd + 1)
                    {
                        tmpStr.push_back(strDoc[i]);
                        i ++;
                    }
                    break;
                }
            }

            // replace '<' with "&lt;" if not matched
            if (!matchName)
            {
                i ++;
                tmpStr.push_back('&');
                tmpStr.push_back('l');
                tmpStr.push_back('t');
                tmpStr.push_back(';');
            }
        }
        else if (ch == '>')
        {
            i ++;
            tmpStr.push_back('&');
            tmpStr.push_back('g');
            tmpStr.push_back('t');
            tmpStr.push_back(';');
        }
        else
        {
            i ++;
            tmpStr.push_back(ch);
        }
    }

    strDoc.swap(tmpStr);
}

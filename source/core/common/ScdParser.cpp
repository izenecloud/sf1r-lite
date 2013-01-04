#include "ScdParser.h"
#include <iostream>
#include <cassert>
#include <cstdio>

#include <util/ustring/UString.h>
#include <util/profiler/ProfilerGroup.h>
#include <util/fileno.hpp>
#include <glog/logging.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/filesystem.hpp>
#define BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_while.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
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

/** remove the trailing '\r' */
inline void removeTrailCarriageReturn(std::string& str)
{
    std::size_t length = str.size();

    if (length > 0 && str[--length] == '\r')
    {
        str.resize(length);
    }
}

}

struct scd_grammar
    : public grammar<scd_grammar>
{
    SCDDoc& scddoc;
    izenelib::util::UString::EncodingType& codingType;
    std::string property_name;

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
        add_property_name(SCDDoc& doc, std::string& propertyname)
            : scddoc(doc), property_name(propertyname) {}
        void operator()(const char *begin, const char *end) const
        {
            property_name.assign(std::string(begin, end));
        }
        SCDDoc& scddoc;
        std::string& property_name;
    };

    struct add_property_value
    {
        add_property_value(SCDDoc& doc, izenelib::util::UString::EncodingType& type, std::string& propertyname)
            : scddoc(doc), codingType(type), property_name(propertyname) {}
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
        std::string& property_name;
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
                           propertyName[add_property_name(self.scddoc,const_cast<std::string&>(self.property_name))]
                           >> ch_p('>') >>
                           propertyValue[add_property_value(self.scddoc,self.codingType,const_cast<std::string&>(self.property_name))];

            // WARN: This rule should also apply to field name on XmlConfigParser.cpp.
            // Those two should share same field name rule to make easy validating.

            propertyName = *(alnum_p | ch_p('-') | ch_p('_'));
            propertyValue = *(~(ch_p('<')));
        }

        const rule<Scanner> &start()
        {
            return object;
        }
    };
};

ScdParser::ScdParser()
    : size_(0), encodingType_(izenelib::util::UString::UTF_8), docDelimiter_(DEFAULT_DOC_DELIMITER)
{}

ScdParser::ScdParser(const izenelib::util::UString::EncodingType & encodingType)
    : size_(0), encodingType_(encodingType), docDelimiter_(DEFAULT_DOC_DELIMITER)
{}

ScdParser::ScdParser(const izenelib::util::UString::EncodingType & encodingType, const char* docDelimiter)
    : size_(0), encodingType_(encodingType), docDelimiter_(docDelimiter)
{}

ScdParser::~ScdParser()
{
    if (fs_.is_open())
    {
        int fd = izenelib::util::fileno(fs_);
        posix_fadvise(fd, 0,0,POSIX_FADV_DONTNEED);
        fs_.close();
    }
}

bool ScdParser::checkSCDFormat(const string & file)
{
    //B-<document collector ID(2Bytes)>-<YYYYMMDDHHmm(12Bytes)>-
    //    <SSsss(second, millisecond, 5Bytes)>-<I/U/D/R(1Byte)>-
    //    <C/F(1Byte)>.SCD

    if ( boost::filesystem::path(file).extension() != ".SCD"
            && boost::filesystem::path(file).extension() != ".scd" )
        return false;

    string fileName = boost::filesystem::path(file).stem().string();

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

unsigned ScdParser::checkSCDDate(const string& file)
{
    static const unsigned kOffset = 5; // strlen("B-01-")
    static const unsigned kCount = 8; // strlen("YYYYMMDD")

    try
    {
        string strVal = boost::filesystem::path(file).stem().string().substr(kOffset, kCount);
        unsigned val = 0;
        sscanf( strVal.c_str(), "%u", &val );
        return val;
    }
    catch (const std::out_of_range&)
    {
        return 0U;
    }
}

unsigned ScdParser::checkSCDTime(const string& file)
{
    static const unsigned kOffset = 13; // strlen("B-01-YYYYMMDD")
    static const unsigned kCount = 10; // strlen("HHmm-SSsss")

    try
    {
        string strVal = boost::filesystem::path(file).stem().string().substr(kOffset, kCount);
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

SCD_TYPE ScdParser::checkSCDType(const string& file)
{
    //B-<document collector ID(2Bytes)>-<YYYYMMDDHHmm(12Bytes)>-
    //    <SSsss(second, millisecond, 5Bytes)>-<I/U/D/R(1Byte)>-
    //    <C/F(1Byte)>.SCD

    string fileName = boost::filesystem::path(file).stem().string();

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

bool ScdParser::compareSCD(const string & file1, const string & file2)
{
    const unsigned date1 = checkSCDDate(file1);
    const unsigned date2 = checkSCDDate(file2);

    if (date1 == date2)
    {
        const unsigned time1 = checkSCDTime(file1);
        const unsigned time2 = checkSCDTime(file2);

        if (time1 == time2)
        {
            return checkSCDType(file1) < checkSCDType(file2);
        }

        return time1 < time2;
    }

    return date1 < date2;
}

void ScdParser::getScdList(const std::string& scd_path, std::vector<std::string>& scd_list)
{
    namespace bfs = boost::filesystem;
    if(!bfs::exists(scd_path)) return;
    if( bfs::is_regular_file(scd_path) && ScdParser::checkSCDFormat(scd_path))
    {
        scd_list.push_back(scd_path);
    }
    else if(bfs::is_directory(scd_path))
    {
        bfs::path p(scd_path);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_regular_file(it->path()))
            {
                std::string file = it->path().string();
                if(ScdParser::checkSCDFormat(file))
                {
                    scd_list.push_back(file);
                }
            }
        }
    }
    std::sort(scd_list.begin(), scd_list.end(), ScdParser::compareSCD);
}


std::size_t ScdParser::getScdDocCount(const std::string& path)
{
    std::vector<std::string> scd_list;
    getScdList(path, scd_list);
    std::size_t count = 0;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string file = scd_list[i];
        std::cerr<<"counting "<<file<<std::endl;
        std::string cmd = "grep -c '<DOCID>' "+file;
        FILE* pipe = popen(cmd.c_str(), "r");
        if(!pipe) 
        {
            std::cerr<<"pipe error"<<std::endl;
            continue;
        }
        char buffer[128];
        std::string result = "";
        while(!feof(pipe))
        {
            if(fgets(buffer, 128, pipe)!=NULL)
            {
                result += buffer;
            }
        }
        pclose(pipe);
        boost::algorithm::trim(result);
        std::size_t c = boost::lexical_cast<std::size_t>(result);
        std::cerr<<"find "<<c<<std::endl;
        count+=c;
    }
    return count;
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

bool ScdParser::getDocIdList(std::vector<izenelib::util::UString> & list)
{
    if ( !fs_.is_open() )
        return false;

    ScdParser::iterator it = this->begin();
    for ( ; it != this->end(); ++it )
    {
        //if the document is NULL
        if ( (*it) == NULL )
        {
            return false;
        }

        list.push_back( (*it)->at(0).second );
        docOffsetList_.insert( (*it)->at(0).second, it.getOffset() );
    }

    return true;
}

bool ScdParser::getDocIdList(std::vector<DocIdPair > & list)
{
    if ( !fs_.is_open() )
        return false;

    ScdParser::iterator it = this->begin();

    for (; it != this->end(); ++it)
    {
        //if the document is NULL
        if ( (*it) == NULL  )
        {
            return false;
        }
        list.push_back( make_pair((*it)->at(0).second, 0) );
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
    return iterator(-1);
}

ScdParser::cached_iterator ScdParser::cbegin(unsigned int start_doc)
{
    fs_.clear();
    fs_.seekg(0, ios::beg);
    return cached_iterator(this, start_doc);
}

ScdParser::cached_iterator ScdParser::cbegin(const std::vector<string>& propertyNameList, unsigned int start_doc)
{
    fs_.clear();
    fs_.seekg(0, ios::beg);
    return cached_iterator(this, start_doc, propertyNameList);
}

ScdParser::cached_iterator ScdParser::cend()
{
    return cached_iterator(-1);
}

bool ScdParser::getDoc(const izenelib::util::UString & docId, SCDDoc& doc)
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
    : pfs_(NULL)
    , prevOffset_(0)
    , offset_(offset)
    , codingType_(izenelib::util::UString::UTF_8)
    , docDelimiter_(DEFAULT_DOC_DELIMITER)
{
}

ScdParser::iterator::iterator(ScdParser* pScdParser, unsigned int start_doc)
    : pfs_(&pScdParser->fs_)
    , prevOffset_(0)
    , offset_(0)
    , codingType_(pScdParser->getEncodingType())
    , buffer_(new izenelib::util::izene_streambuf)
    , docDelimiter_(pScdParser->docDelimiter_)
{
    pfs_->clear();
    std::size_t readLen = izenelib::util::izene_read_until(*pfs_, *buffer_, docDelimiter_);
    if(readLen)
    {
        buffer_->consume(readLen);
        prevOffset_ += readLen - docDelimiter_.size();
        doc_.reset(getDoc());
        operator+=(start_doc);
    }
    else
    {
        offset_ = prevOffset_ = -1;
    }
}

ScdParser::iterator::iterator(ScdParser* pScdParser, unsigned int start_doc, const std::vector<string>& propertyNameList)
    : pfs_(&pScdParser->fs_)
    , prevOffset_(0)
    , offset_(0)
    , codingType_(pScdParser->getEncodingType())
    , buffer_(new izenelib::util::izene_streambuf)
    , docDelimiter_(pScdParser->docDelimiter_)
    , propertyNameList_(propertyNameList)
    , pname_set_(propertyNameList_.begin(), propertyNameList_.end())
{
    pfs_->clear();
    std::size_t readLen = izenelib::util::izene_read_until(*pfs_, *buffer_, docDelimiter_);
    if(readLen)
    {
        buffer_->consume(readLen);
        prevOffset_ += readLen - docDelimiter_.size();
        doc_.reset(getDoc());
        operator+=(start_doc);
    }
    else
    {
        offset_ = prevOffset_ = -1;
    }
}

ScdParser::iterator::iterator(const iterator& other)
    : pfs_(other.pfs_)
    , prevOffset_(other.prevOffset_)
    , offset_(other.offset_)
    , doc_(other.doc_)
    , codingType_(other.codingType_)
    , buffer_(other.buffer_)
    , docDelimiter_(other.docDelimiter_)
{
}

ScdParser::iterator::~iterator()
{
}

const ScdParser::iterator& ScdParser::iterator::operator=(const iterator& other)
{
    pfs_ = other.pfs_;
    prevOffset_ = other.prevOffset_;
    offset_ = other.offset_;
    doc_ = other.doc_;
    codingType_ = other.codingType_;
    buffer_ = other.buffer_;
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

ScdParser::iterator& ScdParser::iterator::operator++()
{
    if (isValid())
    {
        offset_ = prevOffset_;
        doc_.reset(getDoc());
    }
    return *this;
}

ScdParser::iterator ScdParser::iterator::operator++(int)
{
    ScdParser::iterator tmp = *this;
    if (isValid())
    {
        offset_ = prevOffset_;
        doc_.reset(getDoc());
    }
    return tmp;
}

ScdParser::iterator& ScdParser::iterator::operator+=(unsigned int offset)
{
    while (offset && isValid())
    {
        offset_ = prevOffset_;
        doc_.reset(getDoc());

        --offset;
    }
    return *this;
}

const SCDDocPtr& ScdParser::iterator::operator*()
{
    return doc_;
}

long ScdParser::iterator::getOffset()
{
    return offset_;
}

void ScdParser::iterator::parseDoc(std::string& str, SCDDoc* doc)
{
    if (str == docDelimiter_) return;

    //CREATE_PROFILER ( proScdParsing2, "ScdParser", "ScdParsing::getDoc::2");
    //CREATE_PROFILER ( proScdParsing3, "ScdParser", "ScdParsing::getDoc::3");
    //START_PROFILER ( proScdParsing2 );
    ///// It's recommended to handle this processing in application by which SCD is created.
    //preProcessDoc(str);
    //STOP_PROFILER ( proScdParsing2 );

    //START_PROFILER ( proScdParsing3 );
    //scd_grammar g(*doc, codingType_);
    //parse_info<> pi = parse(str.c_str(), g, space_p);
    //STOP_PROFILER ( proScdParsing3 );

    //new start
    CREATE_PROFILER ( proScdParsingN, "ScdParser", "ScdParsing::getDoc::N");
    START_PROFILER ( proScdParsingN );
    static const uint8_t min = 1;
    static const uint8_t max = 20;
    static const std::string white_spaces(" \f\n\r\t\v");

    std::string property_name;
    std::stringstream property_value;
    std::stringstream ss(str);
    std::string line;
    while( getline(ss, line) )
    {
        //boost::algorithm::trim_right(line);
        std::size_t line_len = line.length();
        std::size_t pos = line.find_last_not_of( white_spaces );
        if(pos==std::string::npos)
        {
            line_len = 0;
        }
        else
        {
            line_len = pos+1;
        }
        if(line_len==0) continue;
        bool all_valid = (line_len==line.length())?true:false;
        //LOG(INFO)<<"find line:"<<line_len<<std::endl;
        std::string pname;
        std::string pname_left;
        //to find pname here
        if(line[0]=='<')
        {
            std::size_t right_index = 0;
            for(std::size_t i=1;i<line_len;i++)
            {
                char c = line[i];
                if(c=='>')
                {
                    right_index = i;
                    break;
                }
                else if( (c>='a'&&c<='z') || (c>='A'&&c<='Z') )
                {
                }
                else
                {
                    break;
                }
            }
            if(right_index>0)
            {
                std::size_t len = right_index-1;
                if(len>=min && len<=max)
                {
                    pname = line.substr(1, len);
                    if(pname_set_.empty() || pname_set_.find(pname)!=pname_set_.end())
                    {
                        //LOG(INFO)<<"find pname : "<<pname<<","<<pname.length()<<std::endl;
                        if( right_index<line_len-1 )
                        {
                            pname_left = line.substr(right_index+1, line_len-right_index-1);
                        }
                    }
                    else
                    {
                        //LOG(INFO)<<"invalid pname : "<<pname<<std::endl;
                        pname.clear();
                    }

                }
            }
        }

        if(pname.empty())// not a pname line
        {
            if(!all_valid)
            {
                property_value << "\n" << line.substr(0, line_len);
            }
            else
            {
                property_value << "\n" << line;
            }
        }
        else
        {
            if(!property_name.empty())
            {
                doc->push_back(std::make_pair( property_name, izenelib::util::UString(property_value.str(), codingType_)));
            }
            property_name.clear();
            property_value.str("");
            property_name = pname;
            property_value << pname_left;
        }
    }
    if(!property_name.empty())
    {
        doc->push_back(std::make_pair( property_name, izenelib::util::UString(property_value.str(), codingType_)));
    }
    STOP_PROFILER ( proScdParsingN );
}

SCDDoc* ScdParser::iterator::getDoc()
{
    CREATE_SCOPED_PROFILER ( proScdParsing, "ScdParser", "ScdParsing::getDoc");
    CREATE_PROFILER ( proScdParsing0, "ScdParser", "ScdParsing::getDoc::0");
    CREATE_PROFILER ( proScdParsing1, "ScdParser", "ScdParsing::getDoc::1");

    START_PROFILER ( proScdParsing0 );
    std::size_t readLen = izenelib::util::izene_read_until(*pfs_, *buffer_, docDelimiter_);
    STOP_PROFILER ( proScdParsing0 );
    START_PROFILER ( proScdParsing1 );
    string str(docDelimiter_);
    if (readLen)
    {
        str.append(buffer_->gptr(), readLen - docDelimiter_.size());
        buffer_->consume(readLen);
        prevOffset_ += readLen;
    }
    else
    {
        while ((readLen = izenelib::util::izene_read_until(*pfs_, *buffer_, PATTERN_EOL)))
        {
            str.append(buffer_->gptr(), readLen);
            buffer_->consume(readLen);
        }
        prevOffset_ = -1;
    }
    SCDDoc* doc = new SCDDoc;
    STOP_PROFILER ( proScdParsing1 );
    parseDoc(str, doc);
    //if(true)
    //{
        //std::vector<std::pair<std::string, izenelib::util::UString> >::iterator p;

        //for (p = doc->begin(); p != doc->end(); p++)
        //{
            //const std::string& property_name = p->first;
            //std::string property_value;
            //p->second.convertString(property_value, izenelib::util::UString::UTF_8);
            //std::cout<<property_name<<":"<<property_value<<std::endl;
        //}
    //}
    return doc;
}

bool ScdParser::iterator::isValid() const
{
    return offset_ != -1;
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
    for (size_t i = 0; i < docLen;)
    {
        ch = strDoc[i];

        if (ch == '<')
        {
            // match property name between '<' and '>'
            matchName = false;
            for (size_t t = 0; t < propertyNameList_.size(); t++)
            {
                tagLen = propertyNameList_[t].size();
                tagEnd = i + tagLen + 1;

                if ( tagEnd < docLen && strDoc[tagEnd] == '>'
                        && boost::iequals(strDoc.substr(i + 1, tagLen), propertyNameList_[t])) // case insensitively
                {
                    matchName = true;
                    // keep property name tag
                    tmpStr.append(strDoc, i, tagLen + 2);
                    i = tagEnd + 1;
                    break;
                }
            }

            // replace '<' with "&lt;" if not matched
            if (!matchName)
            {
                i++;
                tmpStr.append("&lt;");
            }
        }
        else if (ch == '>')
        {
            i++;
            tmpStr.append("&gt;");
        }
        else
        {
            i++;
            tmpStr.push_back(ch);
        }
    }

    strDoc.swap(tmpStr);
}

ScdParser::cached_iterator::cached_iterator(long offset):it_(offset)
{
    cache_index_ = 0;
}

ScdParser::cached_iterator::cached_iterator(ScdParser* pParser, uint32_t start_doc):it_(pParser, start_doc)
{
    cache_index_ = 0;
    increment();
}

ScdParser::cached_iterator::cached_iterator(ScdParser* pParser, uint32_t start_doc, const std::vector<std::string>& pname_list):it_(pParser, start_doc, pname_list)
{
    cache_index_ = 0;
    increment();
}

//ScdParser::cached_iterator::cached_iterator(const ScdParser::cached_iterator& other):it_(other.get_iterator())
//{
//}

ScdParser::cached_iterator::~cached_iterator()
{
}

long ScdParser::cached_iterator::getOffset()
{
    if(cache_index_<cache_.size())
    {
        return cache_[cache_index_].second;
    }
    else
    {
        return -1;
    }
    //return offset_;
}

void ScdParser::cached_iterator::increment()
{
    cache_index_++;
    if(cache_index_>=cache_.size())
    {
        //clear read more
        cache_.resize(0);
        cache_index_ = 0;
        for(uint32_t i=0;i<MAX_CACHE_NUM;i++)
        {
            if(it_.getOffset()==-1) break;
            cache_.push_back(std::make_pair(*it_, it_.getOffset()));
            ++it_;
        }

    }
}

const SCDDocPtr ScdParser::cached_iterator::dereference() const
{
    if(cache_index_<cache_.size())
    {
        return cache_[cache_index_].first;
    }
    else
    {
        return SCDDocPtr();
    }
}

bool ScdParser::cached_iterator::equal(const ScdParser::cached_iterator& other) const
{
    return it_ == other.get_iterator() && cache_index_==other.cache_index_ && cache_.size()==other.cache_.size();
}

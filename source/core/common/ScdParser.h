#ifndef __SCD__PARSER__H__
#define __SCD__PARSER__H__

#include <util/ustring/UString.h>
#include <util/streambuf.h>

#include <am/3rdparty/rde_hash.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>


typedef std::pair<izenelib::util::UString, izenelib::util::UString> FieldPair;
typedef std::vector<FieldPair> SCDDoc;
typedef boost::shared_ptr<SCDDoc> SCDDocPtr;
typedef std::pair<izenelib::util::UString, unsigned> DocIdPair;

enum SCD_TYPE
{
    NOT_SCD = 0,
    INSERT_SCD,
    UPDATE_SCD,
    DELETE_SCD
};


class ScdParser
{
    /// @brief  Reads a document from the loaded SCD file, when given a DOCID value.
    //          prerequisites: SCD file must be loaded by load(), and getDocIdList() must be called.
    bool getDoc(const izenelib::util::UString & docId, SCDDoc& doc);

public:
    ScdParser();
    ScdParser(const izenelib::util::UString::EncodingType & encodingType);
    ScdParser(const izenelib::util::UString::EncodingType & encodingType, const char* docDelimiter);
    virtual ~ScdParser();

    static bool checkSCDFormat(const string & file);
    static unsigned checkSCDDate(const string & file);
    static unsigned checkSCDTime(const string & file);
    static SCD_TYPE checkSCDType(const string & file);
    static bool compareSCD(const string & file1, const string & file2);


    /// @brief Release allocated memory and make it as initial state.
    void clear();

    /// @brief Read a SCD file and load data into memory.
    bool load(const std::string& path);

    long getFileSize()
    {
        return size_;
    }

    /// @brief  A utility function to get all the DOCID values from an SCD
    bool getDocIdList(std::vector<izenelib::util::UString> & list);

    bool getDocIdList(std::vector<DocIdPair > & list);


    /// @brief gets the encoding type from the config
    inline izenelib::util::UString::EncodingType& getEncodingType()
    {
        return encodingType_;
    };


    class iterator
    {
    public:
        iterator(long offset);

        iterator(ScdParser* pScdParser, unsigned int start_doc);

        iterator(ScdParser* pScdParser, unsigned int start_doc, const std::vector<string>& propertyNameList);

        iterator(const iterator& other);

        ~iterator();

        const iterator& operator=(const iterator& other);

        bool operator==(const iterator& other) const;

        bool operator!=(const iterator& other) const;

        iterator& operator++();

        iterator operator++(int);

        iterator& operator+=(unsigned int offset);

        SCDDocPtr operator*();

        long getOffset();

    private:
        SCDDoc* getDoc();

        bool isValid() const;

        /// @brief
        /// It's recommended to handle this processing in application by which SCD is created
        void preProcessDoc(string& strDoc);

    private:
        std::ifstream* pfs_;

        long prevOffset_;

        long offset_;

        SCDDocPtr doc_;

        izenelib::util::UString::EncodingType codingType_;

        boost::shared_ptr<izenelib::util::izene_streambuf> buffer_;

        std::string docDelimiter_;

        std::vector<string> propertyNameList_;
    };  // class iterator

    iterator begin(unsigned int start_doc = 0);

    iterator begin(const std::vector<string>& propertyNameList, unsigned int start_doc = 0);

    iterator end();

private:
    ifstream fs_;

    long size_;

    izenelib::util::UString::EncodingType encodingType_;

    izenelib::am::rde_hash<izenelib::util::UString, long> docOffsetList_;

    const std::string docDelimiter_; /// the boundary between each docs
};

#endif

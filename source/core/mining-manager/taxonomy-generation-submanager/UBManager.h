///
/// @file UBManager.h
/// @brief header file of unigram,bigram manager in taxonomy-generation submanager.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-08-27
/// @date Updated 2009-11-21 01:05:43
///

#ifndef UBMANAGER_H_
#define UBMANAGER_H_
#include <string>
#include "TgTypes.h"
#include <3rdparty/febird/io/DataIO.h>
#include <3rdparty/febird/io/StreamBuffer.h>
#include <3rdparty/febird/io/FileStream.h>
namespace sf1r
{


/// @brief storage the unigram information
class UnigramInformation
{
public:
    UnigramInformation(const std::string& path);
    ~UnigramInformation();
private:
    UnigramInformation() {}
    //avoid copy constructor
    UnigramInformation(const UnigramInformation& db) {}
    UnigramInformation& operator=(const UnigramInformation& rhs)
    {
        return *this;
    }
public:
    /// @brief open all cache file.
    void open();
    /// @brief close all cache file.
    void close();

    /// @brief add document frequency for one term id.
    ///
    /// @param termid the term id.
    /// @param freq the frequecy.
    void addDF(termid_t termid, count_t freq = 1);

    /// @brief add term frequency for one term id
    ///
    /// @param termid the term id.
    /// @param freq the frequency.
    void addTF(termid_t termid, count_t freq = 1);

    /// @brief subtract document frequency for one term id
    ///
    /// @param termid the term id.
    /// @param freq the frequency.
    void subDF(termid_t termid, count_t freq = 1);

    /// @brief subtract term frequency for one term id
    ///
    /// @param termid the term id.
    /// @param freq the frequency.
    void subTF(termid_t termid, count_t freq = 1);

    /// @brief get document frequency for one termid.
    ///
    /// @param termid the term id.
    /// @param freq returned frequency.
    ///
    /// @return is this term id existed.
    bool getDF(termid_t termid, count_t& freq);
    /// @brief get document frequency for one termid.
    ///
    /// @param termid the term id.
    /// @param freq returned frequency.
    ///
    /// @return is this term id existed.
    bool getTF(termid_t termid, count_t& freq);

    /// @brief the language information for one term id.
    ///
    /// @param termid the term id.
    /// @param lang the language int.
    ///
    /// @return true.
    bool setLang(termid_t termid, char lang);
    /// @brief get language information for one term id.
    ///
    /// @param termid the term id.
    /// @param lang the returned value.
    ///
    /// @return if this term id existed.
    bool getLang(termid_t termid, char& lang);

    /// @brief reset all terms' count.
    ///
    /// @param count the count.
    void setAllTermCount(count_t count);
    /// @brief get all terms' count.
    ///
    /// @return the count.
    count_t getAllTermCount();
    /// @brief add document count.
    ///
    /// @param count the count.
    void addDocCount(count_t count = 1);
    /// @brief subtract document count.
    ///
    /// @param count the count
    void subDocCount(count_t count = 1);
    /// @brief get documents' count.
    ///
    /// @return the count.
    count_t getDocCount();
    /// @brief flush all information to disk.
    void flush();
private:
    izenelib::am::sdb_fixedhash<termid_t, count_t > unigram_df_;
    izenelib::am::sdb_fixedhash<termid_t, count_t > unigram_tf_;
    izenelib::am::sdb_fixedhash<termid_t, char > unigram_lang_;
    izenelib::am::sdb_fixedhash<bool, count_t > all_term_count_;
    izenelib::am::sdb_fixedhash<bool, count_t > all_doc_count_;
    count_t allTermCount_;
    count_t allDocCount_;
    bool has_change_;
    bool is_open_;
};


}

#endif

///
/// @file QueryLogManager.h
/// @brief The query log manager in MIA
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-11-30 15:59:01
/// @date Updated 2009-11-30 15:59:01
///

#ifndef _QueryLogManager_h_
#define _QueryLogManager_h_

#include <limits>
#include <am/sequence_file/SimpleSequenceFile.hpp>
#include <util/ustring/UString.h>
#include <string>
#include <am/tokyo_cabinet/tc_hash.h>
#include <boost/tuple/tuple.hpp>
#include <common/type_defs.h>
namespace sf1r
{

class QueryLogItem
{

public:
    QueryLogItem(){}
    QueryLogItem( const izenelib::util::UString& str) : str_(str),freq_(1)
    {
        
    }
    izenelib::util::UString str_;
    uint32_t freq_;
    
    QueryLogItem& operator+=(const QueryLogItem& item)
    {
        if(freq_ < std::numeric_limits< uint32_t >::max() - item.freq_)
            freq_ += item.freq_;
        else
        {
            freq_ = std::numeric_limits< uint32_t >::max();
        }
        return *this;
    }
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & str_ & freq_;
    }
};

class QueryLogManager : public boost::noncopyable {
    
typedef uint64_t hash_t;
typedef izenelib::am::SSFType<hash_t, QueryLogItem , uint32_t, false> QueryLogSSFType;    



public:
    
typedef QueryLogSSFType::WriterType writer_t;
typedef QueryLogSSFType::ReaderType reader_t;
typedef QueryLogSSFType::SorterType sorter_t;

typedef izenelib::am::SimpleSequenceFileMerger<hash_t, QueryLogItem, QueryLogItem, uint32_t, uint32_t> merger_t;
    
    QueryLogManager( const std::string& path  );
    ~QueryLogManager();
    
    /**
    * @brief open this QueryLogManager.
    */
    void open();
    
    /**
    * @brief whether is open.
    */
    bool isOpen();
    
    /**
    * @brief close.
    */
    void close();
    
    /**
    * @brief append new query log item
    * @param queryStr the new query string
    */
    void append(const izenelib::util::UString& queryStr);
     
    /**
    * @brief compute for all new query log item.
    */
    reader_t* compute(uint64_t& changeCount);

private:
    void initTimeFile_(bool replace);    
    
private:
    std::string path_;
    bool isOpen_;
    writer_t* writer_;
    boost::mutex writeMutex_;
    const std::string SUM;
    const std::string SUM_TMP;
    const std::string MAIN;
    const std::string TIME;
};
}

#endif // _QueryLogManager_h_

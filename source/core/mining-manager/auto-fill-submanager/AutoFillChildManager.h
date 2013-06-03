/*!
 \file      AutoFillChildManager.h
 \author    Hongliang.Zhao&&Qiang.Wang
 \brief     AutoFillChildManager will get the search item list which match the prefix of the keyword user typed.
 \dat       2012-07-26
*/

#if !defined(_AUTO_FILL_CHILD_SUBMANAGER_)
#define _AUTO_FILL_CHILD_SUBMANAGER_

#include "word_leveldb_table.hpp"

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <log-manager/LogAnalysis.h>
#include <configuration-manager/CollectionPath.h>
#include <log-manager/UserQuery.h>

#include <am/leveldb/Table.h>
#include <am/succinct/wat_array/wat_array.hpp>

#include <ir/id_manager/IDManager.h>
#include <util/cronexpression.h>
#include <util/ThreadModel.h>
#include <util/ustring/UString.h>

#include <vector>
#include <list>
#include <fstream>

namespace izenelib
{
namespace am
{
class QueryNormalize;
}
}

namespace sf1r
{

#define UINT32_LEN sizeof(uint32_t)
#define FREQ_TYPE_LEN sizeof(uint32_t)
#define HITNUM_LEN sizeof(uint32_t)
#define TOPN_LEN sizeof(uint32_t)

class AutoFillChildManager: public boost::noncopyable
{
    typedef uint32_t FREQ_TYPE;
    typedef boost::tuple<uint32_t, uint32_t, izenelib::util::UString> ItemValueType;
    typedef std::pair<uint32_t, izenelib::util::UString> PropertyLabelType;
    typedef izenelib::ir::idmanager::AutoFillIDManager IDManger;
    
    struct PrefixQueryType
    {
        string query_;
        string prefix_;
        FREQ_TYPE freq_;
        uint32_t hitnum_;

        void init(std::string query,string prefix,FREQ_TYPE freq,uint32_t hitnum)
        {
            query_ = query;
            prefix_ = prefix;
            freq_ = freq;
            hitnum_ = hitnum;
        }
        inline bool operator > (const PrefixQueryType other) const
        {
            return prefix_ > other.prefix_ ;
        }

        inline bool operator < (const PrefixQueryType other) const
        {
            return prefix_ < other.prefix_;
        }
        inline bool operator == (const PrefixQueryType other) const
        {
            return prefix_ == other.prefix_;
        }
        QueryType  getQueryType()
        {
            QueryType ret;
            ret.strQuery_=query_;
            ret.freq_=freq_;
            ret.HitNum_=hitnum_;
            return ret;
        };
    };

    struct ItemType
    {
        uint64_t offset_;
        std::string strItem_;

        inline bool operator > (const ItemType other) const
        {
            return strItem_ > other.strItem_ ;
        }

        inline bool operator < (const ItemType other) const
        {
            return strItem_ < other.strItem_;
        }
        inline bool operator > (const std::string& other)
        {
            return strItem_ > other;
        }

        inline bool operator < (const std::string& other)
        {
            return strItem_ < other;
        }

        inline bool operator == (const std::string& other)
        {
            return strItem_ == other;
        }
        inline void operator = (const ItemType& other)
        {
            offset_=other.offset_;
            strItem_=other.strItem_;
        }

        inline bool operator == (const ItemType& other)
        {
            return  (strItem_==other.strItem_);
        }

        inline bool operator != (const ItemType& other)
        {
            return  (strItem_!=other.strItem_);
        }
    };

    struct ValueType
    {
        uint32_t size_;
        std::string  strValue_;
        FREQ_TYPE freq_;
        uint32_t HitNum_;

        void toValueType(const std::string& value, FREQ_TYPE freq, uint32_t HitNum)
        {
            size_ = value.length() + UINT32_LEN*2 + FREQ_TYPE_LEN;
            strValue_ = value;
            freq_ = freq;
            HitNum_ = HitNum;
        }

        void toString(std::string &str)
        {
            char *str_ = new char[size_];
            *(uint32_t*)str_ = size_;
            for(uint32_t i = UINT32_LEN; i < size_ - FREQ_TYPE_LEN - UINT32_LEN; i++)
                str_[i] = strValue_[i - UINT32_LEN];
            *(FREQ_TYPE*)(str_ + size_ - FREQ_TYPE_LEN - UINT32_LEN) = freq_;
            *(uint32_t*)(str_ + size_- UINT32_LEN) = HitNum_;
            string s(str_, size_);
            str = s;
            delete [] str_;
        }

        void setValueType(const char* str)
        {
            size_ = *(uint32_t*)str;
            strValue_.insert(0, str + UINT32_LEN, size_ - UINT32_LEN*2 - FREQ_TYPE_LEN);
            freq_ = *(FREQ_TYPE*)(str + size_ - FREQ_TYPE_LEN - UINT32_LEN);
            HitNum_ = *(uint32_t*)(str + size_ - UINT32_LEN);
        }

        inline void getHitString(std::string &str)
        {
            str = boost::lexical_cast<string>(HitNum_) + "/" + strValue_;
        }
        QueryType  getQueryType()
        {
            QueryType ret;
            ret.strQuery_=strValue_;
            ret.freq_=freq_;
            ret.HitNum_=HitNum_;
            return ret;
        };
    };
    
public:
    AutoFillChildManager(bool isfromSCD = false);

    ~AutoFillChildManager();

    bool Init(const CollectionPath& collectionPath
            , const std::string& collectionName
            , const string& cronExpression
            , const string& instanceName);

    bool InitWhileHaveLeveldb();
    
    bool RealInit();
    
    bool InitFromSCD();
    bool InitFromLog();
    bool InitFromLog_ForTest(std::vector<UserQuery>& query_records);
    bool Init_ForTest(const CollectionPath& collectionPath 
                    , const std::string& collectionName
                    , const string& cronExpression
                    , const string& instanceName
                    , bool & isBuildFromLeveldb
                    , std::vector<UserQuery>& query_records);

    bool PrepareForInit(const CollectionPath& collectionPath 
                        , const std::string& collectionName
                        , const string& cronExpression
                        , const string& instanceName
                        , bool & isBuildFromLeveldb);

    void SaveItem();
    void LoadItem();
    
    void SaveWat();
    bool LoadWat();
    
    bool openDB(std::string path_dbTable, std::string path_dbItem);
    void closeDB();
    void flush();

    void buildDbItem();
    bool buildDbIndex(const std::list<QueryType>& queryList);
    
    void buildDbIndexForEach( std::pair<std::string,std::vector<QueryType> > eachprefix
                        , std::vector<std::pair<string,string> >& similarList);

    void buildDbIndexForEveryThousand(vector<PrefixQueryType> thousandpair
                        , std::vector<std::pair<string,string> >& similarList);

    void buildWat_array(bool _fromleveldb);
    
    bool buildIndex(const std::list<ItemValueType>& queryList);

    void buildItemList(std::string key);
    void buildItemVector();
    void buildTopNDbTable(std::string &value, const uint32_t offset);

    bool getOffset(const std::string& query , uint64_t& OffsetStart, uint64_t& OffsetEnd);

    bool getAutoFillListFromOffset(uint64_t OffsetStart
                                , uint64_t OffsetEnd
                                , std::vector<std::pair<izenelib::util::UString, uint32_t> >& list);

    bool getAutoFillListFromWat(const izenelib::util::UString& query
                            , std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

    void updateAutoFill(int calltype);
    bool getAutoFillListFromDbTable(const izenelib::util::UString& query
                            , std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

    bool getAutoFillList(const izenelib::util::UString& query
                            , std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);
    
    void updateAutoFill();
    void updateFromLog();
    void updateFromLog_ForTest(std::vector<UserQuery>& query_records);
    void updateFromSCD();
    void SaveSCDLog();
    void LoadSCDLog();

    void dealWithSimilar(std::vector<std::pair<string,string> >& SimlarList);
    std::vector<QueryType> valueToQueryTypeVector(string value);
    
    static std::string system_resource_path_;
    static bool enableUpdateHitnum_;

private:
    bool isFromSCD_;
    bool isUpdating_;
    bool isUpdating_Wat_;
    bool isIniting_;

    uint32_t updatelogdays_;
    uint32_t alllogdays_;
    uint32_t topN_;

    std::string collectionName_;
    std::string AutofillPath_;
    std::string leveldbPath_;
    std::string SCDDIC_;
    std::string ItemPath_;
    std::string ItemdbPath_;
    std::string WatArrayPath_;
    std::vector<string> SCDHaveDone_;
    std::string SCDLogPath_;
    std::string cronJobName_;

    boost::mutex buildCollectionMutex_;
    izenelib::util::CronExpression cronExpression_;

    QueryNormalize* QN_;

    WordLevelDBTable dbTable_;
    WordLevelDBTable dbItem_;//

    wat_array::WatArray wa_;

    std::vector<ItemType> ItemVector_;
    boost::scoped_ptr<IDManger> idManager_;
};

} // end - namespace sf1r
#endif // _AUTO_FILL_CHILD_SUBMANAGER_

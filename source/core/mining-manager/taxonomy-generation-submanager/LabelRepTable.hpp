///
/// @file LabelRepTable
/// @brief Provide prefix or suffix search function for label representation.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-01-25
/// @date Updated 2010-01-25
///  --- Log


#ifndef LABELREPTABLE_H_
#define LABELREPTABLE_H_
#include <string>
#include <mining-manager/MiningManagerDef.h>
#include <boost/thread/shared_mutex.hpp>
#include <am/sdb_btree/sdb_bptree.h>
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <am/external_sort/alpha_sort.hpp>
#include <am/db_trie/common_trie.h>
#include "TgTypes.h"
#include "LabelIdCache.h"
#include "GroupInfo.h"

namespace sf1r
{
template <typename ItemType,
          typename NodeIDType,
          typename LockType>
class LabelRepEdgeTable : public izenelib::am::CommonEdgeTable<ItemType, NodeIDType, LockType>
{
    typedef izenelib::am::CommonEdgeTable<ItemType, NodeIDType, LockType> CommonType;
public:
    typedef std::pair<NodeIDType, ItemType> EdgeTableKeyType;
    typedef NodeIDType EdgeTableValueType;
    typedef izenelib::sdb::ordered_sdb_fixed<EdgeTableKeyType, EdgeTableValueType , LockType> DBType;
    LabelRepEdgeTable(const std::string& name)
    : CommonType(name), storage_(name)
    {
    }
    virtual ~LabelRepEdgeTable() { flush(); }
    void open()
    {
        storage_.setCacheSize(10000000);
        storage_.open();
    }
    void close()
    {
        storage_.close();
    }
    void flush()
    {
        storage_.flush();
    }
    void optimize()
    {

    }
    void insertValue(const EdgeTableKeyType& key, const EdgeTableValueType& value)
    {
        storage_.insert(key, value);
    }

    bool getValue(const EdgeTableKeyType& key, EdgeTableValueType& value)
    {
        return storage_.get(key, value);
    }
    
    bool getValueBetween( std::vector<DataType<EdgeTableKeyType,EdgeTableValueType> > & results,
        const EdgeTableKeyType& start, const EdgeTableKeyType& end)
    {
        return storage_.getValueBetween(results, start, end);
    }
    
private:
    DBType storage_;
};


template <typename NodeIDType,
          typename UserDataType,
          typename LockType>
class LabelRepDataTable : public izenelib::am::CommonDataTable<NodeIDType, UserDataType, LockType>
{
    typedef izenelib::am::CommonDataTable<NodeIDType, UserDataType, LockType> CommonType;
    

public:

    typedef typename CommonType::DataTableKeyType DataTableKeyType;
    typedef typename CommonType::DataTableValueType DataTableValueType;
    typedef izenelib::am::tc_hash<DataTableKeyType, DataTableValueType> DBType;

    LabelRepDataTable(const std::string& name)
    : CommonType(name), db_(name) { }

    virtual ~LabelRepDataTable() { flush(); }

    /**
     * @brief Open all db files, maps them to memory. Call it
     *        after config() and before any insert() or find() operations.
     */
    void open()
    {
        db_.setCacheSize(1000000);
        db_.open();
    }

    void flush()
    {
        db_.flush();
    }

    void close()
    {
        db_.close();
    }

    void optimize()
    {
    }

    unsigned int numItems()
    {
        return db_.numItems();
    }

    void insertValue(const DataTableKeyType& key, const DataTableValueType & value)
    {
        db_.insertValue(key, value);
    }

    void update(const DataTableKeyType& key, const DataTableValueType & value)
    {
        db_.update(key, value);
    }

    bool getValue(const DataTableKeyType& key, DataTableValueType & value)
    {
        return db_.getValue(key, value);
    }

    DBType& getDB()
    {
        return db_;
    }

private:

    izenelib::am::tc_hash<DataTableKeyType, DataTableValueType> db_;
};

/**
 * @brief Definitions for distributed tries on each partition.
 */
template <typename StringType,
          typename UserDataType,
          typename NodeIDType = uint64_t,
          typename LockType = izenelib::util::NullLock>
class LabelRepEdgeTrie
{
public:
    typedef typename StringType::value_type CharType;
    typedef NodeIDType IDType;
    typedef LabelRepEdgeTable      <typename StringType::value_type,
                                   NodeIDType,
                                   LockType>
            EdgeTableType;
    typedef typename EdgeTableType::EdgeTableKeyType EdgeTableKeyType;
    typedef typename EdgeTableType::EdgeTableValueType EdgeTableValueType;
    typedef DataType<EdgeTableKeyType, EdgeTableValueType> EdgeTableRecordType;
    typedef LabelRepDataTable      <NodeIDType, UserDataType, LockType>
            DataTableType;
    typedef CommonTrie_<CharType, UserDataType, NodeIDType,
        EdgeTableType, DataTableType> CommonTrieType;
    
//     typedef typename EdgeTableType::EdgeTableKeyType EdgeTableKeyType;
    LabelRepEdgeTrie(const std::string name)
    :   trie_(name) {}


    virtual ~LabelRepEdgeTrie() {}
    
    void open(){ trie_.open(); }
    void flush(){ trie_.flush(); }
    void close(){ trie_.close(); }
    
    
    NodeIDType getRootID() const
    {
        return izenelib::am::NodeIDTraits<NodeIDType>::RootValue;
    }
    
    void insert(const StringType& key, UserDataType value)
    {
        trie_.insert(key, value);
    }
    
    bool findPrefix(const std::vector<CharType>& prefix,
        std::vector<UserDataType>& valueList)
    {
        NodeIDType nid = NodeIDTraits<NodeIDType>::RootValue;
        for( size_t i=0; i<prefix.size(); i++ )
        {
            NodeIDType tmp = NodeIDType();
            if( !getEdge(prefix[i], nid, tmp) )
                return false;
            nid = tmp;
        }
        valueList.clear();
        std::vector<std::vector<CharType> > keyList;

        std::vector<CharType> begin = prefix;
        findPrefix_<CommonTrieType::EnumerateValue>(nid, begin, keyList, valueList);
        return valueList.size() ? true : false;
    }
    
    bool getEdge(const CharType& ch, const NodeIDType& parentNID, NodeIDType& childNID)
    {
        return trie_.getEdge(ch, parentNID, childNID);
    }
    
    bool getData( const NodeIDType& nid, UserDataType& userData)
    {
        return trie_.getData(nid, userData);
    }
    
private:
    
    template<int EnumerateType>
    void findPrefix_( const NodeIDType& nid,
        std::vector<CharType>& prefix,
        std::vector<std::vector<CharType> >& keyList,
        std::vector<UserDataType>& valueList)
    {
        UserDataType tmp = UserDataType();
        if(getData(nid, tmp))
        {
            if(EnumerateType == CommonTrieType::EnumerateKey) {
                keyList.push_back(prefix);
            } else if(EnumerateType == CommonTrieType::EnumerateValue) {
                valueList.push_back(tmp);
            } else {
                keyList.push_back(prefix);
                valueList.push_back(tmp);
            }
        }
        EdgeTableKeyType minKey(nid, izenelib::am::NumericTraits<CharType>::MinValue);
        EdgeTableKeyType maxKey(nid, izenelib::am::NumericTraits<CharType>::MaxValue);
        std::vector<EdgeTableRecordType> result;
        trie_.getEdgeTable().getValueBetween(result, minKey, maxKey);
        for(size_t i = 0; i <result.size(); i++ )
        {
            prefix.push_back(result[i].key.second);
            findPrefix_<EnumerateType>(result[i].value, prefix, keyList, valueList);
            prefix.pop_back();
        }
    }
    
private:

    CommonTrieType trie_;
};

class LabelRepTable
{
    public:
        typedef LabelRepEdgeTrie<std::vector<termid_t>, uint32_t, uint32_t> TrieType;
        typedef TrieType::IDType IDType;
        LabelRepTable(const std::string& name):name_(name), forward_(name+".forward"), reverse_(name+".reverse")
        {
            
        }
        void open()
        {
            forward_.open();
            reverse_.open();
        }
        void flush()
        {
            forward_.flush();
            reverse_.flush();
        }

        void close()
        {
            forward_.close();
            reverse_.close();
        }
        
        void insert(const std::vector<termid_t>& label, uint32_t labelId)
        {
            forward_.insert(label, labelId);
            std::vector<termid_t> reverseVec(label);
            std::reverse(reverseVec.begin(), reverseVec.end());
            reverse_.insert(reverseVec, labelId);
        }
        
        void getRightExtendLabelList(const std::vector<termid_t>& label, std::vector<uint32_t>& labelList)
        {
            forward_.findPrefix(label, labelList);
        }
        
        void getLeftExtendLabelList(const std::vector<termid_t>& label, std::vector<uint32_t>& labelList)
        {
            
            reverse_.findPrefix(label, labelList);
        }
        
        bool getRelativePair(const std::vector<termid_t>& source, izenelib::am::rde_hash<termid_t, bool>& conjTerm,
                              std::vector<std::pair<uint32_t, uint32_t> >& pairList)
        {
            for(uint32_t i=0; i<source.size(); i++)
            {
//                 std::cout<<"S "<<source[i]<<std::endl;
                if( conjTerm.find( source[i] ) != NULL )
                {
//                     std::cout<<"!!!CC "<<i<<" "<<source[i]<<std::endl;
                    uint32_t leftLabelId = 0;
                    uint32_t rightLabelId = 0;
                    bool findLeft = false;
                    bool findRight = false;
                    {
                        //to find left label adject to i
                        if( i>= 1 )
                        {
                            IDType id = reverse_.getRootID();
                            std::vector<IDType> idList;
                            for( int j=i-1;j>=0;j--)
                            {
                                IDType tmp = 0;
//                                 std::cout<<"E1 "<<id<<" "<<j<<" "<<source[j]<<std::endl;
                                if( !reverse_.getEdge(source[j], id, tmp) )
                                {
//                                     std::cout<<"E "<<id<<" "<<j<<" "<<source[j]<<std::endl;
                                    if( idList.size() > 0 )
                                    {
                                        for(int p=idList.size()-1; p>=0;p--)
                                        {
    //                                         std::cout<<"P1 "<<idList[p]<<std::endl;
    //                                         if(false)
                                            if( reverse_.getData(idList[p], leftLabelId) )
                                            {
//                                                 std::cout<<"P "<<idList[p]<<" "<<leftLabelId<<std::endl;
                                                findLeft = true;
                                                break;
                                            }
    //                                         std::cout<<"P2 "<<idList[p]<<std::endl;
                                        }
                                    }
                                    break;
                                }
//                                 std::cout<<"E2 "<<id<<" "<<j<<" "<<source[j]<<std::endl;
                                id = tmp;
                                idList.push_back(id);
                            }
                        }
                    }
//                     std::cout<<"findLeft end"<<std::endl;
                    if( findLeft )
                    {
                        //to find left label adject to i
                        if( i <= source.size()-2 )
                        {
                            IDType id = forward_.getRootID();
                            std::vector<IDType> idList;
                            for( uint32_t j=i+1;j<source.size();j++)
                            {
                                IDType tmp = 0;
                                if( !forward_.getEdge(source[j], id, tmp) )
                                {
                                    if(idList.size()>0)
                                    {
                                        for(int p=idList.size()-1; p>=0;p--)
                                        {
                                            if( forward_.getData(idList[p], rightLabelId) )
                                            {
                                                findRight = true;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                                id = tmp;
                                idList.push_back(id);
                                
                            }
                        }
                    }
                    if( findLeft && findRight )
                    {
                        pairList.push_back( std::make_pair( leftLabelId, rightLabelId ) );
                    }
                }
            }
            return true;
        }
        
    private:
        std::string name_;
        TrieType forward_;
        TrieType reverse_;
};
    

    
}

#endif

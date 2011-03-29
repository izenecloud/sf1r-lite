///
/// @file ConceptIDManager.hpp
/// @brief Label id manager.
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-02-26
/// @date 
///

#ifndef CONCEPTIDMANAGER_HPP_
#define CONCEPTIDMANAGER_HPP_

#include <mining-manager/MiningManagerDef.h>
#include <am/tokyo_cabinet/tc_fixdb.h>
#include <am/sequence_file/SequenceFile.hpp>
#include <util/error_handler.h>
namespace sf1r{

class ConceptIDManager : public izenelib::util::ErrorHandler
{
typedef izenelib::am::SequenceFile<izenelib::util::UString
, izenelib::am::tc_hash<uint32_t, izenelib::util::UString >
, izenelib::am::CommonSeqFileSerializeHandler
, izenelib::am::SeqFileObjectCacheHandler
  > Container;

typedef uint32_t INFO_TYPE;
public:
    ConceptIDManager(const std::string& dir);
    
    ~ConceptIDManager();
    
    void SetSaveExistence(bool b);
    
    bool Open();
    
    bool IsOpen() const;
    
    
    
    void Put(uint32_t id, const izenelib::util::UString& ustr);
    
    bool GetIdByString(const izenelib::util::UString& ustr, uint32_t& id);
    
    inline static uint64_t Hash(const izenelib::util::UString& ustr)
    {
        return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(ustr);
    }

    
    bool GetStringById(uint32_t id, izenelib::util::UString& ustr);

    
    void Flush();
    

    void Close();
    
    size_t size();


    
private:        
    bool isOpen_;
    Container idMap_;
    bool save_existence_;
    izenelib::am::tc_hash<uint64_t, uint32_t> existence_;
    boost::mutex mutex_;
        
};
}

#endif

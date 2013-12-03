#ifndef SF1R_B5MMANAGER_TOURPROCESSOR_H
#define SF1R_B5MMANAGER_TOURPROCESSOR_H 
#include <util/ustring/UString.h>
#include <product-manager/product_price.h>
#include "b5m_helper.h"
#include "b5m_types.h"
#include "b5m_m.h"
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
#include <document-manager/ScdDocument.h>

NS_SF1R_B5M_BEGIN

const static std::string SCD_SOURCE		 = "Source";
const static std::string SCD_PRICE		 = "SalesPrice";
const static std::string SCD_FROM_CITY	 = "FromCity";
const static std::string SCD_TO_CITY	 = "ToCity";
const static std::string SCD_DOC_ID		 = "DOCID";
const static std::string SCD_TIME_PLAN	 = "TimePlan";
const static std::string SCD_UUID		 = "uuid";
const static std::string SCD_NAME		 = "Name";

class TourProcessor{
    typedef izenelib::util::UString UString;
    typedef std::pair<std::string, std::string> BufferKey;
    struct BufferValueItem
    {
        std::string from;
        std::string to;
        std::pair<uint32_t, uint32_t> days;
        double price;
        ScdDocument *doc;
        bool bcluster;
        bool operator<(const BufferValueItem& another) const
        {
            return days<another.days;
        }
    };

    typedef std::vector<BufferValueItem> BufferValue;
    typedef boost::unordered_map<BufferKey, BufferValue> Buffer;
    typedef boost::unordered_set<std::string> Set;
    typedef BufferValue Group;

    typedef std::vector<Document*>	DocVector;
    typedef std::vector<BufferKey>  FromToCityVector;
    //key:FromCity,ToCity  value:collection of documents
    typedef boost::unordered_map<BufferKey,DocVector> FromToHash;
    //key:documents  value:collection of <FromCicy,ToCity>
    typedef boost::unordered_map<Document*,FromToCityVector> DocHash;
    typedef boost::unordered_set<BufferKey> FromToCitySet;
    typedef boost::unordered_set<Document*> DocSet;
    
public:
    TourProcessor(const B5mM& b5mm);
    
    ~TourProcessor();
   
    bool Generate(const std::string& mdb_instance);

private:
    void Insert_(ScdDocument& doc);
    
    std::pair<uint32_t, uint32_t> ParseDays_(const std::string& sdays) const;
    
    void Finish_();
    
    void FindGroups_(BufferValue& value);
    
    void GenP_(Group& g, Document& doc) const;

    /**
     *@brief:log group infomation
     */
    void LogGroup(const DocSet& already_used_docs);
    
    /**
     *@brief:destroy all the duplicate documents
     */
    void Destroy();

    /**
     *@brief:generate tourp group
     */
    void GenerateGroup(DocSet& docs);
    
    /**
     *@brief:search each similar <from,to> collection of documents
     */
    void SearchGroupDocuments(DocSet& already_used_docs,
                              FromToCitySet& already_used_from_to,
                              Document *doc);
    
    /**
     *@brief:aggregate all the similar <from,to> collection of documents
     */
    void AggregateSimilarFromTo();

    /**
     *brief:Generate <from,to>---->{<document>,...}
     * and <document>---->{<from,to>,..}
     */
    void GenerateAuxiliaryHash(Document*doc,
                               const std::string&from_city,
                               const std::string&to_city,
                               const std::string&name);
private:
    B5mM            b5mm_;
    std::string		m_;
    FromToHash		from_to_hash_;
    DocHash		    doc_hash_;
    Buffer		    buffer_;
    boost::mutex    mutex_;
};
NS_SF1R_B5M_END

#endif



#include "simple_data_source.h"

using namespace sf1r;

SimpleDataSource::SimpleDataSource(const PMConfig& config, std::vector<PMDocumentType>* document_list)
:config_(config), document_list_(document_list)
{
    for(uint32_t i=0;i<document_list_->size();i++)
    {
        uint32_t docid = i+1;
        std::string suuid;
        if(!GetUuid_(docid, suuid)) continue;
        UuidIndexType::iterator it = uuid_index_.find(suuid);
        if(it==uuid_index_.end())
        {
            std::vector<uint32_t> value(1, docid);
            uuid_index_.insert(std::make_pair(suuid, value));
        }
        else
        {
            it->second.push_back(docid);
        }
    }
}

SimpleDataSource::~SimpleDataSource()
{
}

bool SimpleDataSource::GetDocument(uint32_t docid, PMDocumentType& doc)
{
    if(docid>document_list_->size()) return false;
    doc = (*document_list_)[docid-1];
    return true;
}

void SimpleDataSource::GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid)
{
    std::string suuid;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    UuidIndexType::iterator it = uuid_index_.find(suuid);
    if(it==uuid_index_.end())
    {
        return;
    }
    else
    {
        std::vector<uint32_t>& value = it->second;
        for(uint32_t i=0;i<value.size();i++)
        {
            if(value[i]!=exceptid)
            {
                docid_list.push_back(value[i]);
            }
        }
    }
}

bool SimpleDataSource::UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid)
{
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        uint32_t docid = docid_list[i];
        std::string suuid;
        if(!GetUuid_(docid, suuid)) return false;
        UuidIndexType::iterator it = uuid_index_.find(suuid);
        if(it==uuid_index_.end())
        {
            return false;
        }
        else
        {
            std::vector<uint32_t>& value = it->second;
            VectorRemove_(value, docid);
        }
    }
    std::string suuid;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    UuidIndexType::iterator it = uuid_index_.find(suuid);
    if(it==uuid_index_.end())
    {
        uuid_index_.insert(std::make_pair(suuid, docid_list));
    }
    else
    {
        std::vector<uint32_t>& value = it->second;
        value.insert(value.end(), docid_list.begin(), docid_list.end());
    }
    return true;
}

bool SimpleDataSource::SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid)
{
    return true;
}

bool SimpleDataSource::GetUuid_(uint32_t docid, izenelib::util::UString& uuid)
{
    const PMDocumentType& doc = (*document_list_)[docid-1];
    PMDocumentType::property_const_iterator it = doc.findProperty(config_.uuid_property_name);
    if(it == doc.propertyEnd())
    {
        return false;
    }
    uuid = it->second.get<izenelib::util::UString>();
    return true;
}
    
bool SimpleDataSource::GetUuid_(uint32_t docid, std::string& suuid)
{
    izenelib::util::UString uuid;
    if(!GetUuid_(docid, uuid)) return false;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    return true;
}


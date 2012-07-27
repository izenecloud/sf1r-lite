#include "simple_data_source.h"

namespace sf1r
{

SimpleDataSource::SimpleDataSource(const PMConfig& config, std::vector<PMDocumentType>* document_list)
    : config_(config), document_list_(document_list)
{
    RebuildIndex_();
}

SimpleDataSource::~SimpleDataSource()
{
}

bool SimpleDataSource::GetDocument(uint32_t docid, PMDocumentType& doc)
{
    if (docid>document_list_->size()) return false;
    doc = (*document_list_)[docid - 1];
    return true;
}

bool SimpleDataSource::AddDocument(uint32_t docid, const PMDocumentType& doc)
{
    if (docid <= document_list_->size()) return false;
    document_list_->resize(docid);
    (*document_list_)[docid - 1] = doc;
    RebuildIndex_();
    return true;
}

bool SimpleDataSource::UpdateDocument(uint32_t docid, const PMDocumentType& doc)
{
    if (docid > document_list_->size()) return false;
    (*document_list_)[docid - 1] = doc;
    RebuildIndex_();
    return true;
}

bool SimpleDataSource::DeleteDocument(uint32_t docid)
{
    if (docid>document_list_->size()) return false;
    (*document_list_)[docid - 1] = PMDocumentType();
    RebuildIndex_();
    return true;
}

void SimpleDataSource::RebuildIndex_()
{
    uuid_index_.clear();
    for (uint32_t i = 0; i < document_list_->size(); i++)
    {
        uint32_t docid = i+1;
        std::string suuid;
        if (!GetUuid_(docid, suuid)) continue;
        UuidIndexType::iterator it = uuid_index_.find(suuid);
        if (it == uuid_index_.end())
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

void SimpleDataSource::GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid)
{
    std::string suuid;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    UuidIndexType::iterator it = uuid_index_.find(suuid);
    if (it == uuid_index_.end())
    {
        return;
    }
    else
    {
        std::vector<uint32_t>& value = it->second;
        docid_list.reserve(value.size());
        for (uint32_t i = 0; i < value.size(); i++)
        {
            if (value[i] != exceptid)
            {
                docid_list.push_back(value[i]);
            }
        }
    }
}

bool SimpleDataSource::UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid)
{
//     std::string suuid;
//     uuid.convertString(suuid, izenelib::util::UString::UTF_8);
//     std::cout<<"UpdateUuid to "<<suuid<<std::endl;
//     for (uint32_t i=0;i<docid_list.size();i++)
//     {
//         std::cout<<"docid "<<docid_list[i]<<std::endl;
//     }
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        uint32_t docid = docid_list[i];
        PMDocumentType doc;
        if (!GetDocument(docid, doc)) return false;
        (*document_list_)[docid - 1].property(config_.uuid_property_name) = uuid;
    }
    RebuildIndex_();
    return true;
}

bool SimpleDataSource::SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid)
{
    return true;
}

bool SimpleDataSource::GetUuid_(uint32_t docid, izenelib::util::UString& uuid)
{
    const PMDocumentType& doc = (*document_list_)[docid - 1];
    PMDocumentType::property_const_iterator it = doc.findProperty(config_.uuid_property_name);
    if (it == doc.propertyEnd())
    {
        return false;
    }
    uuid = it->second.get<izenelib::util::UString>();
    return true;
}

bool SimpleDataSource::GetUuid_(uint32_t docid, std::string& suuid)
{
    izenelib::util::UString uuid;
    if (!GetUuid_(docid, uuid)) return false;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    return true;
}

bool SimpleDataSource::GetPrice(const PMDocumentType& doc, ProductPrice& price) const
{
    izenelib::util::UString price_ustr;
    if (!doc.getProperty(config_.price_property_name, price_ustr)) return false;
    return price.Parse(price_ustr);
}

bool SimpleDataSource::GetPrice(const uint32_t& docid, ProductPrice& price) const
{
    return GetPrice((*document_list_)[docid - 1], price);
}

}

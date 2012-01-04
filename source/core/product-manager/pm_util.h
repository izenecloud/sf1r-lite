#ifndef SF1R_PRODUCTMANAGER_PMUTIL_H
#define SF1R_PRODUCTMANAGER_PMUTIL_H

#include <common/type_defs.h>
// #include <document-manager/Document.h>
#include "pm_config.h"
#include "pm_types.h"
#include "product_price.h"
#include "product_data_source.h"
namespace sf1r
{

class PMUtil
{
    
public:
    
    PMUtil(const PMConfig& config, ProductDataSource* data_source)
    : config_(config), data_source_(data_source)
    {
    }
    
    bool GetPrice(uint32_t docid, ProductPrice& price) const
    {
        PMDocumentType doc;
        if( !data_source_->GetDocument(docid, doc)) return false;
        return GetPrice(doc, price);
    }
    
    bool GetPrice(const PMDocumentType& doc, ProductPrice& price) const
    {
        izenelib::util::UString price_ustr;
        if(!doc.getProperty(config_.price_property_name, price_ustr)) return false;
        return price.Parse(price_ustr);
    }
    
    void GetPrice(const std::vector<uint32_t>& docid_list, ProductPrice& price) const
    {
        std::vector<PMDocumentType> doc_list;
        doc_list.reserve(docid_list.size());
        
        for (uint32_t i = 0; i < docid_list.size(); i++)
        {
            PMDocumentType doc;
            if( data_source_->GetDocument(docid_list[i], doc))
            {
                doc_list.push_back(doc);
            }
        }
        GetPrice(doc_list, price);
    }

    void GetPrice(const std::vector<PMDocumentType>& doc_list, ProductPrice& price) const
    {
        for (uint32_t i = 0; i < doc_list.size(); i++)
        {
            ProductPrice p;
            if (GetPrice(doc_list[i], p))
            {
                price += p;
            }
        }
    }
    
    
    void AddPrice( PMDocumentType& doc, const ProductPrice& price ) const
    {
        ProductPrice doc_price;
        GetPrice(doc, doc_price);
        doc_price += price;
        doc.property(config_.price_property_name) = doc_price.ToUString();
    }
    
    void AddPrice( PMDocumentType& doc, const PMDocumentType& from ) const
    {
        ProductPrice price;
        if( GetPrice(from, price ) )
        {
            ProductPrice doc_price;
            GetPrice(doc, doc_price);
            doc_price += price;
            doc.property(config_.price_property_name) = doc_price.ToUString();
        }
    }
    
    void SetItemCount( PMDocumentType& doc, uint32_t count)
    {
        doc.property(config_.itemcount_property_name) = izenelib::util::UString(boost::lexical_cast<std::string>(count), izenelib::util::UString::UTF_8);
    }

    void SetManmade(PMDocumentType& doc)
    {
        doc.property(config_.manmade_property_name) = izenelib::util::UString("1", izenelib::util::UString::UTF_8);

    }
    
    uint32_t GetUuidDf(const izenelib::util::UString& uuid)
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        return same_docid_list.size();
    }
    
private:
    
    PMConfig config_;
    ProductDataSource* data_source_;
};

}

#endif

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
        return data_source_->GetPrice(docid, price);
    }

    bool GetPrice(const PMDocumentType& doc, ProductPrice& price) const
    {
        return data_source_->GetPrice(doc, price);
    }

    void GetPrice(const std::vector<uint32_t>& docid_list, ProductPrice& price) const
    {
        for (uint32_t i = 0; i < docid_list.size(); i++)
        {
            ProductPrice p;
            if (GetPrice(docid_list[i], p))
            {
                price += p;
            }
        }
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

    void AddPrice(PMDocumentType& doc, const ProductPrice& price) const
    {
        ProductPrice doc_price;
        GetPrice(doc, doc_price);
        doc_price += price;
        doc.property(config_.price_property_name) = doc_price.ToPropString();
    }

    void AddPrice(PMDocumentType& doc, const PMDocumentType& from) const
    {
        ProductPrice price;
        if (GetPrice(from, price))
        {
            ProductPrice doc_price;
            GetPrice(doc, doc_price);
            doc_price += price;
            doc.property(config_.price_property_name) = doc_price.ToPropString();
        }
    }

    void AddSource(izenelib::util::UString& combine_source, const izenelib::util::UString& source)
    {
        if(source.length()>0)
        {
            std::string s_combine;
            combine_source.convertString(s_combine, izenelib::util::UString::UTF_8);
            std::string s_source;
            source.convertString(s_source, izenelib::util::UString::UTF_8);
            std::size_t start = 0, end = 0;
            //bool dd = false; //if source is in combine_source already
            while(true)
            {
                end = s_combine.find(",", start);
                std::size_t param2 = std::string::npos;
                if(end!=std::string::npos)
                {
                    param2 = end - start;
                }
                std::string s = s_combine.substr(start, param2);
                if(s_source==s)
                {
                    return;
                }
                if(end==std::string::npos) break;
                start = end+1;
            }

            if(combine_source.length()>0)
            {
                combine_source.push_back(',');
            }
            combine_source.append(source);
        }

    }

    void SetItemCount(PMDocumentType& doc, uint32_t count)
    {
        doc.property(config_.itemcount_property_name) = str_to_propstr(boost::lexical_cast<std::string>(count));
    }

    void SetManmade(PMDocumentType& doc)
    {
        doc.property(config_.manmade_property_name) = str_to_propstr("1");

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

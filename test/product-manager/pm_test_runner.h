#ifndef SF1R_TEST_PMTESTRUNNER_H
#define SF1R_TEST_PMTESTRUNNER_H

#include "pm_test_parser.h"
#include <common/type_defs.h>


#include <document-manager/DocumentManager.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include <product-manager/pm_def.h>
#include <product-manager/pm_types.h>
#include <product-manager/pm_config.h>
#include <product-manager/product_price.h>
#include <common/Utilities.h>
#include <boost/test/unit_test.hpp>

using namespace sf1r;

class PMTestRunner
{
public:
    PMTestRunner()
        : pm_config_()
        , document_list_(new std::vector<PMDocumentType>())
        , data_source_(new SimpleDataSource(pm_config_, document_list_))
        , price_trend_(NULL)
        , pm_(new ProductManager("./", boost::shared_ptr<DocumentManager>(), data_source_, price_trend_, pm_config_))
    {
    }

    ~PMTestRunner()
    {
        delete document_list_;
        delete data_source_;
        delete pm_;
    }

    void Test(const std::vector<std::string>& input_list, const std::vector<std::string>& result_list)
    {
        for(uint32_t i=0;i<input_list.size();i++)
        {
            std::string line = input_list[i];
            std::vector<std::string> input_vec;
            PMTestParser::Split(line, ',', input_vec);
            BOOST_CHECK_MESSAGE( !input_vec.empty(), "invalid input "+line);
            BOOST_CHECK_MESSAGE( input_vec[0].length()==1, "invalid input "+line);
            char first = input_vec[0][0];
            if(first == 'I')
            {
                PMDocumentType doc;
                PMTestParserI::Parse(line, doc);
                I_(doc);
            }
            else if(first=='U')
            {
                PMDocumentType doc;
                izenelib::ir::indexmanager::IndexerDocument index_document;
                bool r_type = false;
                PMTestParserU::Parse(line, doc, index_document, r_type);
                U_(doc, index_document, r_type);
            }
            else if(first=='D')
            {
                uint32_t docid = 0;
                PMTestParserD::Parse(line, docid);
                D_(docid);
            }
            else
            {
                BOOST_CHECK_MESSAGE( false, "invalid input "+line);
            }
        }
        BOOST_CHECK( pm_->FinishHook(Utilities::createTimeStamp()) );
    }

    void ShowAllDocs()
    {
        for(uint32_t i=0;i<document_list_->size();i++)
        {
            PMDocumentType& doc = (*document_list_)[i];
            std::cout<<doc.getId()<<",";
            PMDocumentType::property_iterator it = doc.propertyBegin();
            while( it!=doc.propertyEnd())
            {
                std::cout<<it->first<<":";
                std::string str = propstr_to_str(it->second.getPropertyStrValue());
                std::cout<<str<<",";
                ++it;
            }
            std::cout<<std::endl;
        }

    }

private:

    void I_(PMDocumentType& doc)
    {
        uint32_t docid = doc.getId();
        if(docid > document_list_->size())
        {
//             document_list_->resize(docid);
        }
        else
        {
            return;//not new
        }
        BOOST_CHECK( pm_->HookInsert(doc, Utilities::createTimeStamp()) );
        BOOST_CHECK( data_source_->AddDocument(docid, doc) );
//         (*document_list_)[docid-1] = doc;
    }

    void U_(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type)
    {
        uint32_t docid = doc.getId();
        uint32_t oldid = index_document.getOldId();
        if(oldid > document_list_->size()) return;
        if(r_type && docid>document_list_->size() ) return;
        if(!r_type && docid<=document_list_->size() ) return;
        BOOST_CHECK( pm_->HookUpdate(doc, index_document.getOldId(), Utilities::createTimeStamp()) );
        if(r_type)
        {
            BOOST_CHECK( data_source_->UpdateDocument(docid, doc) );
//             (*document_list_)[docid-1] = doc;
        }
        else
        {
//             document_list_->resize(docid);
            BOOST_CHECK( data_source_->AddDocument(docid, doc) );
            BOOST_CHECK( data_source_->DeleteDocument(oldid) );
//             (*document_list_)[docid-1] = doc;
//             (*document_list_)[oldid-1] = PMDocumentType();
        }

    }

    void D_(uint32_t docid)
    {
        if(docid > document_list_->size()) return;
        BOOST_CHECK( pm_->HookDelete(docid, Utilities::createTimeStamp()) );
        BOOST_CHECK( data_source_->DeleteDocument(docid) );
//         (*document_list_)[docid-1] = PMDocumentType();
    }

    void GetUuid_(uint32_t docid, izenelib::util::UString& uuid)
    {
        BOOST_CHECK( document_list_->size()>=docid);
        PMDocumentType& doc = (*document_list_)[docid-1];
        PMDocumentType::property_iterator it = doc.findProperty(pm_config_.uuid_property_name);
        BOOST_CHECK( it != doc.propertyEnd());
        uuid = propstr_to_ustr(it->second.getPropertyStrValue());
    }

    void ResultCheck_(const PMTestResultItem& item, const std::pair<SCD_TYPE, PMDocumentType>& result)
    {
        BOOST_CHECK( item.op == result.first );
        const PMDocumentType& result_doc = result.second;
        if(item.price.value.first>=0.0)
        {
            PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.price_property_name);
            BOOST_CHECK( it != result_doc.propertyEnd());
            Document::doc_prop_value_strtype uprice = it->second.getPropertyStrValue();
            ProductPrice price;
            price.Parse(propstr_to_ustr(uprice));
            BOOST_CHECK( item.price == price );
        }
        if(item.itemcount>0)
        {
            PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.itemcount_property_name);
            BOOST_CHECK( it != result_doc.propertyEnd());
            Document::doc_prop_value_strtype uic = it->second.getPropertyStrValue();
            std::string sic = propstr_to_str(uic);
            uint32_t ic = boost::lexical_cast<uint32_t>(sic);
            BOOST_CHECK( item.itemcount == ic );
        }
        //check uuid.
        if(item.uuid_docid>0)
        {
            BOOST_CHECK( document_list_->size()>=item.uuid_docid);
            Document::doc_prop_value_strtype source_uuid;
            Document::doc_prop_value_strtype uuid;
            {
                PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.docid_property_name);
                BOOST_CHECK( it != result_doc.propertyEnd());
                source_uuid = it->second.getPropertyStrValue();
            }
            {
                PMDocumentType& doc = (*document_list_)[item.uuid_docid-1];
                PMDocumentType::property_const_iterator it = doc.findProperty(pm_config_.uuid_property_name);
                BOOST_CHECK( it != doc.propertyEnd());
                uuid = it->second.getPropertyStrValue();
            }
//             std::cout<<"check uuid "<<item.uuid_docid<<","<<uuid<<","<<source_uuid<<std::endl;
            BOOST_CHECK( uuid == source_uuid );
        }
    }

private:
    PMConfig pm_config_;
    std::vector<PMDocumentType>* document_list_;
    SimpleDataSource* data_source_;
    ProductPriceTrend* price_trend_;
    ProductManager* pm_;

};

#endif

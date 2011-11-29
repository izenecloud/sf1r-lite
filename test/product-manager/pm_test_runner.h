#ifndef SF1R_TEST_PMTESTRUNNER_H
#define SF1R_TEST_PMTESTRUNNER_H

#include "pm_test_parser.h"
#include <common/type_defs.h>


#include <document-manager/Document.h>
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
        , op_processor_(new SimpleOperationProcessor())
        , price_trend_(NULL)
        , pm_(new ProductManager(data_source_, op_processor_, price_trend_, pm_config_))
    {
    }

    ~PMTestRunner()
    {
        delete document_list_;
        delete data_source_;
        delete op_processor_;
        delete pm_;
    }

    void Test(const std::vector<std::string>& input_list, const std::vector<std::string>& result_list)
    {
        std::vector<std::pair<int, PMDocumentType> >& source = op_processor_->Data();
        uint32_t source_start = source.size();
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
            else if(first=='C')
            {
                std::vector<uint32_t> docid_list;
                bool succ = false;
                PMTestParserC::Parse(line, docid_list, succ);
                C_(docid_list, succ);
            }
            else if(first=='A')
            {
                uint32_t uuid_docid = 0;
                std::vector<uint32_t> docid_list;
                bool succ = false;
                PMTestParserA::Parse(line, uuid_docid, docid_list, succ);
                A_(uuid_docid, docid_list, succ);
            }
            else if(first=='R')
            {
                uint32_t uuid_docid = 0;
                std::vector<uint32_t> docid_list;
                bool succ = false;
                PMTestParserR::Parse(line, uuid_docid, docid_list, succ);
                R_(uuid_docid, docid_list, succ);
            }
            else
            {
                BOOST_CHECK_MESSAGE( false, "invalid input "+line);
            }
        }
        BOOST_CHECK( pm_->Finish() );
        Validation_(source_start, result_list);
//         ShowAllDocs();
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
                std::string str;
                it->second.get<izenelib::util::UString>().convertString(str, izenelib::util::UString::UTF_8);
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
        izenelib::ir::indexmanager::IndexerDocument index_document;
        BOOST_CHECK( pm_->HookInsert(doc, index_document, Utilities::createTimeStamp()) );
        BOOST_CHECK( data_source_->AddDocument(docid, doc) );
//         (*document_list_)[docid-1] = doc;
    }

    void U_(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type)
    {
        uint32_t docid = doc.getId();
        uint32_t oldid = index_document.getId();
        if(oldid > document_list_->size()) return;
        if(r_type && docid>document_list_->size() ) return;
        if(!r_type && docid<=document_list_->size() ) return;
        BOOST_CHECK( pm_->HookUpdate(doc, index_document, Utilities::createTimeStamp(), r_type) );
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

    void C_(const std::vector<uint32_t>& docid_list, bool succ)
    {
        izenelib::util::UString gen_uuid;
        bool bsucc = pm_->AddGroup(docid_list, gen_uuid);
        if(!bsucc)
        {
            std::cout<<pm_->GetLastError()<<std::endl;
        }
        BOOST_CHECK( bsucc==succ);
    }

    void GetUuid_(uint32_t docid, izenelib::util::UString& uuid)
    {
        BOOST_CHECK( document_list_->size()>=docid);
        PMDocumentType& doc = (*document_list_)[docid-1];
        PMDocumentType::property_iterator it = doc.findProperty(pm_config_.uuid_property_name);
        BOOST_CHECK( it != doc.propertyEnd());
        uuid = it->second.get<izenelib::util::UString>();
    }

    void A_(uint32_t uuid_docid, const std::vector<uint32_t>& docid_list, bool succ)
    {
        izenelib::util::UString uuid;
        GetUuid_(uuid_docid, uuid);
        bool bsucc = pm_->AppendToGroup(uuid, docid_list);
        if(!bsucc)
        {
            std::cout<<pm_->GetLastError()<<std::endl;
        }
        BOOST_CHECK( bsucc==succ);
    }

    void R_(uint32_t uuid_docid, const std::vector<uint32_t>& docid_list, bool succ)
    {
        izenelib::util::UString uuid;
        GetUuid_(uuid_docid, uuid);
        bool bsucc = pm_->RemoveFromGroup(uuid, docid_list);
        if(!bsucc)
        {
            std::cout<<pm_->GetLastError()<<std::endl;
        }
        BOOST_CHECK( bsucc==succ);
    }

    void Validation_(uint32_t start, const std::vector<std::string>& result_list)
    {
        std::vector<std::pair<int, PMDocumentType> >& all_source = op_processor_->Data();
        std::vector<std::pair<int, PMDocumentType> > source(all_source.begin()+start, all_source.end());
        BOOST_CHECK(source.size()==result_list.size());

        for(uint32_t i=0;i<result_list.size();i++)
        {
            PMTestResultItem item;
            PMTestParser::ParseResult(result_list[i], item);
            ResultCheck_(item, source[i]);
        }
    }

    void ResultCheck_(const PMTestResultItem& item, const std::pair<int, PMDocumentType>& result)
    {
        BOOST_CHECK( item.op == result.first );
        const PMDocumentType& result_doc = result.second;
        if(item.price.value.first>=0.0)
        {
            PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.price_property_name);
            BOOST_CHECK( it != result_doc.propertyEnd());
            izenelib::util::UString uprice = it->second.get<izenelib::util::UString>();
            ProductPrice price;
            price.Parse(uprice);
            BOOST_CHECK( item.price == price );
        }
        if(item.itemcount>0)
        {
            PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.itemcount_property_name);
            BOOST_CHECK( it != result_doc.propertyEnd());
            izenelib::util::UString uic = it->second.get<izenelib::util::UString>();
            std::string sic;
            uic.convertString(sic, izenelib::util::UString::UTF_8);
            uint32_t ic = boost::lexical_cast<uint32_t>(sic);
            BOOST_CHECK( item.itemcount == ic );
        }
        //check uuid.
        if(item.uuid_docid>0)
        {
            BOOST_CHECK( document_list_->size()>=item.uuid_docid);
            izenelib::util::UString source_uuid;
            izenelib::util::UString uuid;
            {
                PMDocumentType::property_const_iterator it = result_doc.findProperty(pm_config_.docid_property_name);
                BOOST_CHECK( it != result_doc.propertyEnd());
                source_uuid = it->second.get<izenelib::util::UString>();
            }
            {
                PMDocumentType& doc = (*document_list_)[item.uuid_docid-1];
                PMDocumentType::property_const_iterator it = doc.findProperty(pm_config_.uuid_property_name);
                BOOST_CHECK( it != doc.propertyEnd());
                uuid = it->second.get<izenelib::util::UString>();
            }
//             std::cout<<"check uuid "<<item.uuid_docid<<","<<uuid<<","<<source_uuid<<std::endl;
            BOOST_CHECK( uuid == source_uuid );
        }
    }

private:
    PMConfig pm_config_;
    std::vector<PMDocumentType>* document_list_;
    SimpleDataSource* data_source_;
    SimpleOperationProcessor* op_processor_;
    ProductPriceTrend* price_trend_;
    ProductManager* pm_;

};

#endif

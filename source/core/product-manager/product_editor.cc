#include "product_editor.h"
#include "product_data_source.h"
#include "operation_processor.h"
#include "uuid_generator.h"
#include <glog/logging.h>
#include <common/Utilities.h>

#define USE_LOG_SERVER 

#ifdef USE_LOG_SERVER
#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>
#endif
using namespace sf1r;

#define PM_EDIT_INFO

ProductEditor::ProductEditor(ProductDataSource* data_source, OperationProcessor* op_processor,const PMConfig& config)
: data_source_(data_source), op_processor_(op_processor), config_(config), util_(config, data_source)
{
}

ProductEditor::~ProductEditor()
{
}

//update documents in A, so need transfer
bool ProductEditor::UpdateADoc(const PMDocumentType& doc)
{
    UString uuid;
    if( !doc.getProperty(config_.docid_property_name, uuid) )
    {
        error_ = "Input doc does not have DOCID property";
        return false;
    }
    if (doc.hasProperty(config_.price_property_name))
    {
        error_ = "Can not update Price property manually.";
        return false;
    }
    if (doc.hasProperty(config_.itemcount_property_name))
    {
        error_ = "Can not update itemcount property manually.";
        return false;
    }
    std::vector<uint32_t> same_docid_list;
    data_source_->GetDocIdList(uuid, same_docid_list, 0);
    if(same_docid_list.empty())
    {
        std::string suuid;
        uuid.convertString(suuid, izenelib::util::UString::UTF_8);
        error_ = "UUID "+suuid+" not exist";
        return false;
    }
    std::vector<PMDocumentType> doc_list;
    return AppendToGroup_(doc_list, doc);
}

//all intervention functions.
bool ProductEditor::AddGroup(const std::vector<uint32_t>& docid_list, PMDocumentType& info, const ProductEditOption& option)
{
    LOG(INFO)<<"call AddGroup"<<std::endl;
    if (docid_list.size()<2)
    {
        error_ = "Docid list size must larger than 1";
        return false;
    }
    UString uuid;
    if( info.getProperty(config_.docid_property_name, uuid) )
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        if (!same_docid_list.empty())
        {
            std::string suuid;
            uuid.convertString(suuid, izenelib::util::UString::UTF_8);
            error_ = "UUID "+suuid+" was exists";
            return false;
        }
    }
    else
    {
        while(true)
        {
            UuidGenerator::Gen(uuid);
            std::vector<uint32_t> same_docid_list;
            data_source_->GetDocIdList(uuid, same_docid_list, 0);
            if (same_docid_list.empty())
            {
                break;
            }
        }
        info.property(config_.docid_property_name) = uuid;
    }
    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }

    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid_list[i], same_docid_list, docid);
        if (!same_docid_list.empty())
        {
            if( !option.force )
            {
#ifdef PM_EDIT_INFO
                for(uint32_t s=0;s<same_docid_list.size();s++)
                {
                    LOG(INFO)<<"Uuid in : "<<uuid_list[i]<<" , "<<same_docid_list[s]<<std::endl;
                }
#endif
                error_ = "Document id "+boost::lexical_cast<std::string>(docid_list[i])+" belongs to other group";
                return false;
            }
            else
            {
                LOG(WARNING)<<"Document id "<<docid_list[i]<<" belongs to other group";

            }
        }
    }

    return AppendToGroup_(doc_list, info);
}

bool ProductEditor::AppendToGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        if (same_docid_list.empty())
        {
            std::string suuid;
            uuid.convertString(suuid, izenelib::util::UString::UTF_8);
            error_ = "UUID "+suuid+" was not exists";
            return false;
        }
    }
    PMDocumentType info;
    info.property(config_.docid_property_name) = uuid;

    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }

    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid_list[i], same_docid_list, docid);
        if (!same_docid_list.empty())
        {
            if( !option.force )
            {
                error_ = "Document id "+boost::lexical_cast<std::string>(docid_list[i])+" belongs to other group";
                return false;
            }
            else
            {
                LOG(WARNING)<<"Document id "<<docid_list[i]<<" belongs to other group";
            }
        }
    }

    return AppendToGroup_(doc_list, info);
}

bool ProductEditor::AppendToGroup_(const std::vector<PMDocumentType>& doc_list,  const PMDocumentType& info)
{
#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
#endif

    //do not make any check in this function
    UString uuid;
    info.getProperty(config_.docid_property_name, uuid);
    PMDocumentType new_doc(info);
    if(doc_list.size()>0)
    {
        //use the first document info as the group info by default
        new_doc.copyPropertiesFromDocument(doc_list[0], false);
    }
    new_doc.eraseProperty(config_.price_property_name);
    new_doc.eraseProperty(config_.uuid_property_name);
    std::vector<uint32_t> uuid_docid_list;
    int type = 1; //insert
    {
        data_source_->GetDocIdList(uuid, uuid_docid_list, 0);
        if (!uuid_docid_list.empty())
        {
            type = 2; //update
        }
    }
    for(uint32_t i=0;i<doc_list.size();i++)
    {
        const PMDocumentType& doc = doc_list[i];
        uint32_t docid = doc.getId();
        UString doc_uuid;
        doc.getProperty(config_.uuid_property_name, doc_uuid);
        std::string old_docids_inuuid;
        //
        if(!doc_uuid.empty() && doc_uuid != uuid)
        {
            std::string scd_docid;
            doc.getString("DOCID", scd_docid);
            LOG(WARNING)<<"Document id "<< scd_docid <<" group has been changed from " <<
               doc_uuid << " to new group:" << uuid << endl;
            // add old group info to this docid's history groups (docid, ..., uuid_list[i])
            data_source_->AddCurUuidToHistory(docid);
#ifdef USE_LOG_SERVER
            // add old docid to the old group's history docids (uuid_list[i], ..., docid)
            AddOldUUIDRequest add_uuidreq;
            AddOldDocIdRequest add_docidreq;
            add_uuidreq.param_.docid_ = Utilities::uuidToUint128(scd_docid);
            doc_uuid.convertString(add_uuidreq.param_.olduuid_, UString::UTF_8);
            add_docidreq.param_.uuid_ = Utilities::uuidToUint128(doc_uuid);
            add_docidreq.param_.olddocid_ = scd_docid;
            OldDocIdData docid_rsp;
            OldUUIDData  uuid_rsp;
            conn.syncRequest(add_uuidreq, uuid_rsp);
            conn.syncRequest(add_docidreq, docid_rsp);

            old_docids_inuuid = docid_rsp.olddocid_;
#endif
        }

        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(doc_uuid, same_docid_list, docid);
        // deletion will only happen when the docid is actually deleted
        // moving to another group will never cause an deletion
        //
        //if( same_docid_list.empty())
        //{
        //    //this docid not belongs to any group

        //    //need to delete
        //    PMDocumentType del_doc;
        //    del_doc.property(config_.docid_property_name) = doc_uuid;
//#ifdef PM_EDIT_INFO
        //    LOG(INFO)<<"Output : "<<3<<" , "<<doc_uuid<<std::endl;
//#endif
        //    op_processor_->Append(3, del_doc);
        //    util_.AddPrice(new_doc, doc);

        //}
        //else
        {
            ProductPrice update_price;
            util_.GetPrice(same_docid_list, update_price);
            PMDocumentType update_doc;
            update_doc.property(config_.docid_property_name) = doc_uuid;
            if(!old_docids_inuuid.empty())
            {
                update_doc.property(config_.oldofferids_property_name) = UString(old_docids_inuuid, UString::UTF_8);
            }
            uint32_t itemcount = same_docid_list.size();
            util_.SetItemCount(update_doc, itemcount);
            util_.AddPrice(update_doc, update_price);
#ifdef PM_EDIT_INFO
            LOG(INFO)<<"Output : "<<2<<" , "<<doc_uuid<<" , itemcount: "<<itemcount<<std::endl;
#endif
            op_processor_->Append(2, update_doc);
            util_.AddPrice(new_doc, doc);
        }


        //update managers.
        std::vector<uint32_t> update_docid_list(1, docid);
#ifdef PM_EDIT_INFO
        LOG(INFO)<<"Updating uuid : "<<docid<<" , "<<uuid<<std::endl;
#endif
        if (!data_source_->UpdateUuid(update_docid_list, uuid))
        {
            error_ = "Update uuid failed";
            return false;
        }
#ifdef USE_LOG_SERVER
        UpdateUUIDRequest uuidReq;
        uuidReq.param_.uuid_ = Utilities::uuidToUint128(doc_uuid);
        for(uint32_t s=0;s<same_docid_list.size();s++)
        {
            PMDocumentType sdoc;
            if(!data_source_->GetDocument(same_docid_list[s], sdoc)) continue;
            izenelib::util::UString sdocid;
            sdoc.getProperty(config_.docid_property_name, sdocid);
            uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocid));
        }
        conn.asynRequest(uuidReq);
#endif
    }
    data_source_->Flush();
    uint32_t itemcount = util_.GetUuidDf(uuid);
    util_.SetItemCount(new_doc, itemcount);
    util_.SetManmade(new_doc);
#ifdef PM_EDIT_INFO
    LOG(INFO)<<"Output : "<<type<<" , "<<uuid<<" , itemcount: "<<itemcount<<std::endl;
#endif
    op_processor_->Append(type, new_doc);
#ifdef USE_LOG_SERVER
    UpdateUUIDRequest uuidReq;
    uuidReq.param_.uuid_ = Utilities::uuidToUint128(uuid);
    for(uint32_t s=0;s<doc_list.size();s++)
    {
        izenelib::util::UString sdocid;
        doc_list[s].getProperty(config_.docid_property_name, sdocid);
        uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocid));
    }
    conn.asynRequest(uuidReq);
#endif
    return true;
}

// remove should only be called when the document will be deleted later
// if just want to change the group please call another instead.
bool ProductEditor::RemovePermanentlyFromAnyGroup(const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
#endif
    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }
    if(doc_list.empty())
    {
        if( !option.force )
        {
            error_ = "Empty document list.";
            return false;
        }
        else
        {
            LOG(WARNING)<<"Empty document list."<<std::endl;
            return true;
        }
    }
    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
    }

    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        std::string s_oldgroups;
        doc_list[i].getString(config_.olduuid_property_name, s_oldgroups);
        std::set<string> s_oldgroups_set;
        boost::algorithm::split(s_oldgroups_set, s_oldgroups, boost::is_any_of(","));
        std::string s_curuuid;
        uuid_list[i].convertString(s_curuuid, UString::UTF_8);
        s_oldgroups_set.insert(s_curuuid);

        std::set<string>::iterator it = s_oldgroups_set.begin();
        std::vector<uint128_t> olduuid_list;
        while(it != s_oldgroups_set.end())
        {
            if((*it).empty())
            {
                ++it;
               continue;
            }
            olduuid_list.push_back(Utilities::md5ToUint128(*it));
            ++it;
        }

#ifdef USE_LOG_SERVER
        std::string scd_docid;
        doc_list[i].getString(config_.docid_property_name, scd_docid);

        DelOldDocIdRequest del_docidreq;
        del_docidreq.param_.uuid_list_ = olduuid_list;
        del_docidreq.param_.olddocid_ = scd_docid;
        // delete should be sync
        DelOldDocIdData rsp;
        conn.syncRequest(del_docidreq, rsp);
#endif
        // remove all its history groups 
        doc_list[i].eraseProperty(config_.olduuid_property_name);
        data_source_->Flush();

        // check all the history groups and the current group in the docid to see
        // if any of these group can be deleted (empty group)
        it = s_oldgroups_set.begin();
        while(it != s_oldgroups_set.end())
        {
            UString olduuid = UString(*it, UString::UTF_8);
            std::string history_docid_list;
            // get the history docid list in the group. if history is also empty
            // then this group can be deleted.
#ifdef USE_LOG_SERVER
            GetOldDocIdRequest get_docidreq;
            get_docidreq.param_.uuid_ = Utilities::uuidToUint128(olduuid);
            OldDocIdData rsp;
            conn.syncRequest(get_docidreq, rsp);
            history_docid_list = rsp.olddocid_;
            // except the deleting docid.
            history_docid_list.replace(history_docid_list.find(scd_docid),
              scd_docid.size(), "");
#endif
            // get the current docid list in the group, if both history and
            // current docid are empty, that means no more items in this group,
            // the group can be safely deleted.
            PMDocumentType origin_doc;
            origin_doc.property(config_.docid_property_name) = olduuid;
            std::vector<uint32_t> same_docid_list;
            data_source_->GetDocIdList(olduuid, same_docid_list, 0);
            if (same_docid_list.empty() && history_docid_list.empty())
            {
                //all deleted
#ifdef PM_EDIT_INFO
                LOG(INFO)<<"Output : "<<3<<" , "<< olduuid <<std::endl;
#endif
                op_processor_->Append(3, origin_doc);
            }
            else
            {
                ProductPrice price;
                util_.GetPrice(same_docid_list, price);
                origin_doc.property(config_.price_property_name) = price.ToUString();
                uint32_t itemcount = same_docid_list.size();
                util_.SetItemCount(origin_doc, itemcount);
                util_.SetManmade(origin_doc);
#ifdef PM_EDIT_INFO
                LOG(INFO)<<"Output : "<<2<<" , "<< olduuid <<" , itemcount: "<<itemcount<<std::endl;
#endif
                op_processor_->Append(2, origin_doc);
            }
#ifdef USE_LOG_SERVER
            UpdateUUIDRequest uuidReq;
            uuidReq.param_.uuid_ = Utilities::uuidToUint128(olduuid);
            for(uint32_t s=0;s<same_docid_list.size();s++)
            {
                PMDocumentType sdoc;
                if(!data_source_->GetDocument(same_docid_list[s], sdoc)) continue;
                izenelib::util::UString sdocid;
                sdoc.getProperty(config_.docid_property_name, sdocid);
                uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocid));
            }
            conn.asynRequest(uuidReq);
#endif
        }
    }
    return true;
}

bool ProductEditor::RemoveFromGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
#endif
    {
        std::vector<uint32_t> same_docid_list;
        data_source_->GetDocIdList(uuid, same_docid_list, 0);
        if (same_docid_list.empty())
        {
            std::string suuid;
            uuid.convertString(suuid, izenelib::util::UString::UTF_8);
            error_ = "UUID "+suuid+" was not exists";
            return false;
        }
    }
    std::vector<PMDocumentType> doc_list;
    doc_list.reserve(docid_list.size());
    for (uint32_t i = 0; i < docid_list.size(); i++)
    {
        PMDocumentType doc;
        if (!data_source_->GetDocument(docid_list[i], doc))
        {
            if( !option.force )
            {
                error_ = "Can not get document "+boost::lexical_cast<std::string>(docid_list[i]);
                return false;
            }
            else
            {
                LOG(WARNING)<<"Can not get document "<<docid_list[i]<<std::endl;
            }
        }
        else
        {
            doc_list.push_back(doc);
        }
    }
    if(doc_list.empty())
    {
        if( !option.force )
        {
            error_ = "Empty document list.";
            return false;
        }
        else
        {
            LOG(WARNING)<<"Empty document list."<<std::endl;
            return true;
        }
    }
    std::vector<UString> uuid_list(doc_list.size());
    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();
        if (!doc_list[i].getProperty(config_.uuid_property_name, uuid_list[i]))
        {
            error_ = "Can not get uuid in document "+boost::lexical_cast<std::string>(docid);
            return false;
        }
        if( uuid_list[i] != uuid )
        {
            error_ = "Uuid in document "+boost::lexical_cast<std::string>(docid)+" does not match the request uuid";
            return false;
        }
    }

    for (uint32_t i = 0; i < doc_list.size(); i++)
    {
        uint32_t docid = doc_list[i].getId();

        // add the removing group info to this docid's history groups (docid, ..., uuid_list[i])
        data_source_->AddCurUuidToHistory(docid);

        PMDocumentType new_doc(doc_list[i]);
        izenelib::util::UString doc_uuid;
        UuidGenerator::Gen(doc_uuid);
        new_doc.property(config_.docid_property_name) = doc_uuid;
        new_doc.eraseProperty(config_.uuid_property_name);
        util_.SetItemCount(new_doc, 1);
 
#ifdef PM_EDIT_INFO
        LOG(INFO)<<"Output : "<<1<<" , "<<doc_uuid<<" , itemcount: "<<1<<std::endl;
#endif
        op_processor_->Append(1, new_doc);
        std::vector<uint32_t> update_docid_list(1, docid);
#ifdef PM_EDIT_INFO
        LOG(INFO)<<"Updating uuid : "<<docid<<" , "<<doc_uuid<<std::endl;
#endif
        if (!data_source_->UpdateUuid(update_docid_list, doc_uuid))
        {
            error_ = "Update uuid failed";
            return false;
        }
#ifdef USE_LOG_SERVER
        UpdateUUIDRequest uuidReq;
        uuidReq.param_.uuid_ = Utilities::uuidToUint128(doc_uuid);
        izenelib::util::UString docname;
        doc_list[i].getProperty(config_.docid_property_name, docname);
        uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(docname));
        conn.asynRequest(uuidReq);
        // add the removing group info to this docid's history groups (docid, ..., uuid_list[i])
        // add old docid to the removing group's history docids (uuid_list[i], ..., docid)
        AddOldUUIDRequest add_uuidreq;
        AddOldDocIdRequest add_docidreq;
        std::string scd_docid;
        doc_list[i].getString(config_.docid_property_name, scd_docid);
        std::string s_uuid;
        uuid.convertString(s_uuid, UString::UTF_8);
        add_uuidreq.param_.docid_ = Utilities::uuidToUint128(scd_docid);
        add_uuidreq.param_.olduuid_ = s_uuid;
        add_docidreq.param_.uuid_ = Utilities::uuidToUint128(uuid);
        add_docidreq.param_.olddocid_ = scd_docid;
        OldDocIdData docid_rsp;
        OldUUIDData uuid_rsp;
        conn.syncRequest(add_uuidreq, uuid_rsp);
        conn.syncRequest(add_docidreq, docid_rsp);
#endif
    }
    std::string old_docids_inuuid;

#ifdef USE_LOG_SERVER
    GetOldDocIdRequest get_docidreq;
    get_docidreq.param_.uuid_ = Utilities::uuidToUint128(uuid);
    OldDocIdData rsp;
    conn.syncRequest(get_docidreq, rsp);
    old_docids_inuuid = rsp.olddocid_;
#endif

    data_source_->Flush();
    PMDocumentType origin_doc;
    origin_doc.property(config_.docid_property_name) = uuid;
    if(!old_docids_inuuid.empty())
    {
        origin_doc.property(config_.oldofferids_property_name) = UString(old_docids_inuuid, UString::UTF_8);
    }
    std::vector<uint32_t> same_docid_list;
    data_source_->GetDocIdList(uuid, same_docid_list, 0);

    // deletion will only happen when the docid is actually deleted
    // moving to another group or just removing from a group will never cause an deletion
    //if (same_docid_list.empty())
    //{
    //    //all deleted
//#ifdef PM_EDIT_INFO
    //    LOG(INFO)<<"Output : "<<3<<" , "<<uuid<<std::endl;
//#endif
    //    op_processor_->Append(3, origin_doc);
    //}
    //else
    {
        ProductPrice price;
        util_.GetPrice(same_docid_list, price);
        origin_doc.property(config_.price_property_name) = price.ToUString();
        uint32_t itemcount = same_docid_list.size();
        util_.SetItemCount(origin_doc, itemcount);
        util_.SetManmade(origin_doc);
#ifdef PM_EDIT_INFO
        LOG(INFO)<<"Output : "<<2<<" , "<<uuid<<" , itemcount: "<<itemcount<<std::endl;
#endif
        op_processor_->Append(2, origin_doc);
    }
#ifdef USE_LOG_SERVER
    UpdateUUIDRequest uuidReq;
    uuidReq.param_.uuid_ = Utilities::uuidToUint128(uuid);
    for(uint32_t s=0;s<same_docid_list.size();s++)
    {
        PMDocumentType sdoc;
        if(!data_source_->GetDocument(same_docid_list[s], sdoc)) continue;
        izenelib::util::UString sdocid;
        sdoc.getProperty(config_.docid_property_name, sdocid);
        uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocid));
    }
    conn.asynRequest(uuidReq);
#endif
    return true;
}

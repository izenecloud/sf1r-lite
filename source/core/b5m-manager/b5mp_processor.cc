#include "b5mp_processor.h"
#include "log_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <configuration-manager/LogServerConnectionConfig.h>

using namespace sf1r;

B5mpProcessor::B5mpProcessor(const std::string& mdb_instance,
    const std::string& last_mdb_instance, BrandDb* bdb)
: mdb_instance_(mdb_instance), last_mdb_instance_(last_mdb_instance), bdb_(bdb)
{
}

bool B5mpProcessor::Generate()
{
    //if(historydb_)
    //{
        //if(!historydb_->is_open())
        //{
            //if(!historydb_->open())
            //{
                //LOG(ERROR)<<"B5MHistoryDBHelper open fail"<<std::endl;
                //return false;
            //}
        //}
    //}
    //else
    //{
        //if(log_server_cfg_ == NULL)
        //{
            //LOG(ERROR)<<"log server config empty"<<std::endl;
            //return false;
        //}
        //// use logserver instead of local history 
        //if(!LogServerClient::Init(*log_server_cfg_))
        //{
            //LOG(ERROR) << "Log Server Init failed." << std::endl;
            //return false;
        //}
        //if(!LogServerClient::Test())
        //{
            //LOG(ERROR) << "log server test failed" << std::endl;
            //return false;
        //}
    //}

    if(bdb_!=NULL)
    {
        if(!bdb_->is_open())
        {
            if(!bdb_->open())
            {
                LOG(ERROR)<<"bdb open error"<<std::endl;
                return false;
            }
        }
    }
    ScdMerger::PropertyConfig config;
    config.output_dir = B5MHelper::GetB5mpPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(config.output_dir);
    config.property_name = "uuid";
    config.merge_function = boost::bind( &B5mpProcessor::ProductMerge_, this, _1, _2);
    config.output_function = boost::bind( &B5mpProcessor::ProductOutput_, this, _1, _2);
    config.output_if_no_position = false;
    std::string b5mo_mirror = B5MHelper::GetB5moMirrorPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(b5mo_mirror);
    ScdMerger merger(b5mo_mirror, true);
    merger.AddPropertyConfig(config);
    if(!last_mdb_instance_.empty())
    {
        std::string mirror = B5MHelper::GetB5moMirrorPath(last_mdb_instance_);
        ScdMerger::InputSource is;
        is.scd_path = mirror;
        is.position = false;
        merger.AddInput(is);
    }
    ScdMerger::InputSource is;
    is.scd_path = B5MHelper::GetB5moPath(mdb_instance_);
    is.position = true;
    merger.AddInput(is);
    if(!last_mdb_instance_.empty())
    {
        uint32_t m = ScdParser::getScdDocCount(is.scd_path)/1200000+1;
        merger.SetModSplit(m);
    }

    //po_map_writer_ = new PoMapWriter(B5MHelper::GetPoMapPath(mdb_instance_));
    //po_map_writer_->Open();
    merger.Output();
    //po_map_writer_->Close();
    //delete po_map_writer_;
    //po_map_writer_ = NULL;
    if(bdb_!=NULL)
    {
        bdb_->flush();
    }
    return true;
}


void B5mpProcessor::ProductMerge_(ScdMerger::ValueType& value, const ScdMerger::ValueType& another_value)
{
    //value is pdoc or empty, another_value is odoc
    ProductProperty pp;
    if(!value.empty())
    {
        pp.Parse(value.doc);
    }
    ProductProperty another;
    another.Parse(another_value.doc);
    //UString anotherdocid, another_uuid, curdocid,curuuid;

    //std::string spid;
    //another.pid.convertString(spid, izenelib::util::UString::UTF_8);
    if(another_value.type!=DELETE_SCD)
    {
        //if(spid=="403ec13d9939290d24c308b3da250658")
        //{
            //LOG(INFO)<<pp.ToString()<<std::endl;
        //}
        pp += another;
        //if(spid=="403ec13d9939290d24c308b3da250658")
        //{
            //LOG(INFO)<<pp.ToString()<<std::endl;
        //}
        if(value.empty() || another.oid==another.pid )
        {
            //value.doc = another_value.doc;
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
        }
        else
        {
            value.doc.copyPropertiesFromDocument(another_value.doc, false);
        }
        value.type = UPDATE_SCD;
    }
    else
    {
        //pp.itemcount-=1;
        if(pp.pid.empty())
        {
            pp.pid = another.pid;
        }
        //if(spid=="403ec13d9939290d24c308b3da250658")
        //{
            //LOG(INFO)<<pp.ToString()<<std::endl;
        //}
    }
    pp.Set(value.doc);

    //add to po_map
    //uint128_t ipid = B5MHelper::UStringToUint128(another.pid);
    //std::string oid;
    //if(another_value.type!=DELETE_SCD)
    //{
        //another.oid.convertString(oid, izenelib::util::UString::UTF_8);
    //}
    //po_map_writer_->Append(ipid, oid);

}

void B5mpProcessor::ProductOutput_(Document& doc, int& type)
{
    doc.eraseProperty("OID");
    int64_t itemcount = 0;
    doc.getProperty("itemcount", itemcount);
    UString ptitle;
    doc.getProperty(B5MHelper::GetSPTPropertyName(), ptitle);
    if(!ptitle.empty())
    {
        doc.property("Title") = ptitle;
        doc.eraseProperty(B5MHelper::GetSPTPropertyName());
    }

    //UString docid;
    //std::string docid_s;
    //doc.getProperty("DOCID", docid);
    //docid.convertString(docid_s, izenelib::util::UString::UTF_8);
    //std::string offerids_s;
    //if(historydb_)
    //{
        //historydb_->pd_get(docid_s, offerids_s);
    //}
    //else
    //{
        //LogServerClient::GetOldDocIdList(docid_s, offerids_s);
    //}
    //doc.property("OldOfferIds") = UString(offerids_s, UString::UTF_8);

    if(itemcount<=0)
    {
        //if(!offerids_s.empty())
        //{
            //cout << "===== itemcount = 0 but old offers not empty, docid:" << 
                //docid_s << "offerids:" << offerids_s <<endl;
            //cout << "this product would be reserved for keep consistent." << endl;
            //return;
        //}

        type = DELETE_SCD;
    }
    if(type==DELETE_SCD)
    {
        doc.clearExceptDOCID();
    }
    else if(bdb_!=NULL)
    {
        UString spid;
        doc.getProperty("DOCID", spid);
        uint128_t pid = B5MHelper::UStringToUint128(spid);
        UString brand;
        doc.getProperty(B5MHelper::GetBrandPropertyName(), brand);
        if(brand.length()>0)
        {
            bdb_->set(pid, brand);
        }
    }
}


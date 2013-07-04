#include "b5mp_processor.h"
#include "log_server_client.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

B5mpProcessor::B5mpProcessor(const std::string& mdb_instance,
    const std::string& last_mdb_instance)
: mdb_instance_(mdb_instance), last_mdb_instance_(last_mdb_instance), odoc_count_(0)
{
    //random_properties will not be updated while SPU matched, they're not SPU related
    random_properties_.push_back("Content");
    random_properties_.push_back("OriginalCategory");
    //random_properties_.push_back("Picture");//remove if we use SPU picture in future.
    random_properties_.push_back("TargetCategory");
    //random_properties_.push_back("Url");
}

bool B5mpProcessor::Generate()
{
    std::string b5mo_path = B5MHelper::GetB5moPath(mdb_instance_);
    PairwiseScdMerger merger(b5mo_path);
    odoc_count_ = ScdParser::getScdDocCount(b5mo_path);
    LOG(INFO)<<"o doc count "<<odoc_count_<<std::endl;
    uint32_t m = odoc_count_/2400000+1;
    if(!last_mdb_instance_.empty())
    {
        std::string last_b5mo_mirror = B5MHelper::GetB5moMirrorPath(last_mdb_instance_);
        merger.SetExistScdPath(last_b5mo_mirror);
        m = odoc_count_/1200000+1;
    }
    merger.SetM(m);
    merger.SetMProperty("uuid");
    std::string output_dir = B5MHelper::GetB5moMirrorPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(output_dir);
    merger.SetOutputPath(output_dir);
    merger.SetOutputer(boost::bind( &B5mpProcessor::B5moOutput_, this, _1, _2));
    merger.SetMEnd(boost::bind( &B5mpProcessor::OutputAll_, this));
    std::string p_output_dir = B5MHelper::GetB5mpPath(mdb_instance_);
    B5MHelper::PrepareEmptyDir(p_output_dir);
    writer_.reset(new ScdTypeWriter(p_output_dir));
    if(!last_mdb_instance_.empty())
    {
        filter_.reset(new FilterType(odoc_count_+100, 0.0000000001));
        ofilter_.reset(new FilterType(odoc_count_+100, 0.0000000001));
        merger.SetPreprocessor(boost::bind( &B5mpProcessor::FilterTask_, this, _1));
    }
    merger.Run();
    writer_->Close();
    return true;
}

void B5mpProcessor::FilterTask_(ValueType& value)
{
    Document& doc = value.doc;
    uint128_t pid = GetPid_(doc);
    if(pid==0) return;
    uint128_t oid = GetOid_(doc);
    if(oid==0) return;
    filter_->Insert(pid);
    ofilter_->Insert(oid);
}

bool B5mpProcessor::B5moValid_(const Document& doc)
{
    uint128_t pid = GetPid_(doc);
    if(pid==0) return false;
    uint128_t oid = GetOid_(doc);
    if(oid==0) return false;
    if(!last_mdb_instance_.empty())
    {
        if(filter_->Get(pid)||ofilter_->Get(oid))
        {
            //LOG(INFO)<<"valid b5mo "<<B5MHelper::Uint128ToString(oid)<<std::endl;
            return true;
        }
        else return false;
    }
    else return true;

}

void B5mpProcessor::B5moPost_(ValueType& value, int status)                     
{
    if(value.type==DELETE_SCD) value.type = NOT_SCD;
    else
    {
        value.type = UPDATE_SCD;//I,U,R
    }
    //if(status==PairwiseScdMerger::OLD) value.type=NOT_SCD;
}

void B5mpProcessor::B5moOutput_(ValueType& value, int status)
{
    uint128_t pid = GetPid_(value.doc);
    if(pid==0)
    {
        B5moPost_(value, status);
        return;
    }
    if(!B5moValid_(value.doc)) 
    {
        B5moPost_(value, status);
        return;
    }
    bool independent_mode = false;
    uint128_t oid = GetOid_(value.doc);
    std::string spu_title;
    value.doc.getString(B5MHelper::GetSPTPropertyName(), spu_title);
    if(oid==pid && spu_title.empty() && last_mdb_instance_.empty()) independent_mode = true;
    if(!independent_mode)
    {
        PValueType& pvalue = cache_[pid];
        if(status==PairwiseScdMerger::OLD||status==PairwiseScdMerger::EXIST)
        {
            ProductMerge_(pvalue.first, value);
        }
        if(status!=PairwiseScdMerger::OLD)
        {
            ProductMerge_(pvalue.second, value);
        }
    }
    else
    {
        PValueType pvalue;
        ProductMerge_(pvalue.second, value);
        ProductOutput_(pvalue);
    }
    B5moPost_(value, status);
}

void B5mpProcessor::ProductMerge_(ValueType& value, const ValueType& another_value)
{
    //value is pdoc or empty, another_value is odoc
    ProductProperty pp;
    if(!value.empty())
    {
        pp.Parse(value.doc);
    }
    ProductProperty another;
    another.Parse(another_value.doc);
    if(another_value.type!=DELETE_SCD)
    {
        pp += another;
        if(value.empty() || another.oid==another.pid )
        {
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
        }
        else
        {
            const PropertyValue& docid_value = value.doc.property("DOCID");
            //override if empty property
            for (Document::property_const_iterator it = another_value.doc.propertyBegin(); it != another_value.doc.propertyEnd(); ++it)
            {
                if (!value.doc.hasProperty(it->first))
                {
                    value.doc.property(it->first) = it->second;
                }
                else
                {
                    PropertyValue& pvalue = value.doc.property(it->first);
                    if(pvalue.which()==docid_value.which()) //is UString
                    {
                        const PropertyValue::PropertyValueStrType& uvalue = pvalue.getPropertyStrValue();
                        if(uvalue.empty())
                        {
                            pvalue = it->second;
                        }
                    }
                }
            }
        }
        value.type = UPDATE_SCD;
    }
    else
    {
        if(pp.pid.empty())
        {
            pp.pid = another.pid;
        }
    }
    pp.Set(value.doc);


}
void B5mpProcessor::OutputAll_()
{
    for(CacheIterator it = cache_.begin();it!=cache_.end();it++)
    {
        PValueType& pvalue = it->second;
        ProductOutput_(pvalue);
    }
    cache_.clear();
}
void B5mpProcessor::GetOutputP_(ValueType& value)
{
    if(value.empty())
    {
        value.type = NOT_SCD;
        return;
    }
    Document& doc = value.doc;
    SCD_TYPE& type = value.type;
    doc.eraseProperty("OID");
    int64_t itemcount = 0;
    doc.getProperty("itemcount", itemcount);
    Document::doc_prop_value_strtype ptitle;
    doc.getProperty(B5MHelper::GetSPTPropertyName(), ptitle);
    if(!ptitle.empty())
    {
        doc.property("Title") = ptitle;
        doc.eraseProperty(B5MHelper::GetSPTPropertyName());
    }

    if(itemcount<=0)
    {
        type = DELETE_SCD;
    }
    if(type==DELETE_SCD)
    {
        doc.clearExceptDOCID();
    }
    if(type!=DELETE_SCD)
    {
        if(doc.getPropertySize()<2)
        {
            value.type = NOT_SCD;
        }
    }
}

void B5mpProcessor::ProductOutput_(PValueType& pvalue)
{
    ValueType& value = pvalue.second;
    ValueType& evalue = pvalue.first;
    GetOutputP_(evalue);
    GetOutputP_(value);
    if(value.empty()) return;
    if(value.type==NOT_SCD) return;
    if(!evalue.empty()&&value.type==UPDATE_SCD)
    {
        Document& doc = value.doc;
        Document& edoc = evalue.doc;
        bool independent = false;
        int64_t ii = 0;
        doc.getProperty("independent", ii);
        if(ii>0) independent = true;
        if(!independent)
        {
            for(uint32_t i=0;i<random_properties_.size();i++)
            {
                doc.eraseProperty(random_properties_[i]);
            }
        }
        for(Document::property_iterator it = edoc.propertyBegin();it!=edoc.propertyEnd();it++)
        {
            const std::string& pname = it->first;
            if(pname=="DOCID") continue;
            PropertyValue& ev = it->second;
            if(doc.hasProperty(pname))
            {
                PropertyValue& v = doc.property(pname);
                if(v==ev)
                {
                    doc.eraseProperty(pname);
                }
            }
        }
        if(doc.getPropertySize()<2)
        {
            value.type = NOT_SCD;
        }
        else if(doc.getPropertySize()==2)//DATE only update
        {
            if(doc.hasProperty("DATE"))
            {
                value.type = NOT_SCD;
            }
        }
    }
    writer_->Append(value.doc, value.type);
}
uint128_t B5mpProcessor::GetPid_(const Document& doc)
{
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty()) return 0;
    return B5MHelper::StringToUint128(spid);
}
uint128_t B5mpProcessor::GetOid_(const Document& doc)
{
    std::string soid;
    doc.getString("DOCID", soid);
    if(soid.empty()) return 0;
    return B5MHelper::StringToUint128(soid);
}


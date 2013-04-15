#ifndef SF1R_COMMON_SCDMERGER_H_
#define SF1R_COMMON_SCDMERGER_H_
#include <types.h>
#include <algorithm>
#include <am/succinct/fujimap/fujimap.hpp>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdTypeWriter.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <am/range/AmIterator.h>
//#include <am/sequence_file/ssfr.h>
#include <glog/logging.h>

//#define MERGER_DEBUG

namespace sf1r
{

class ScdMerger
{
public:
    
    struct ValueType
    {
        ValueType():type(NOT_SCD)
        {
        }

        ValueType(const Document& p1, SCD_TYPE p2)
        :doc(p1), type(p2)
        {
        }

        Document doc;
        SCD_TYPE type;

        bool empty() const
        {
            return doc.isEmpty();
        }

        void swap(ValueType& other)
        {
            std::swap(type, other.type);
            doc.swap(other.doc);
        }

    };
    typedef boost::function<void (ValueType&, const ValueType&) > MergeFunctionType;
    typedef boost::function<void (ValueType&) > Processor;
    //typedef boost::function<void (ValueType&,int) > Outputer;
    typedef boost::function<void ()> MEndFunction;

    typedef uint128_t IdType;
    typedef uint32_t PositionType;

    typedef boost::unordered_map<IdType, ValueType> CacheType;
    typedef CacheType::iterator CacheIterator;
    //typedef izenelib::am::succinct::fujimap::Fujimap<IdType, PositionType> PositionMap;





    

    
public:

    ScdMerger(const std::string& scd_path): m_(0), scd_path_(scd_path), mproperty_("DOCID"), all_type_(false)
    {
    }


    void SetMProperty(const std::string& name)
    {
        mproperty_ = name;
    }

    void SetM(uint32_t m)
    {
        LOG(INFO)<<"set mod split "<<m<<std::endl;
        m_ = m;
    }

    void SetAllType(bool b=true)
    {
        all_type_ = b;
    }

    void SetOutputPath(const std::string& path)
    {
        boost::filesystem::create_directories(path);
        if(writer_) writer_.reset();
        if(!path.empty())
        {
            writer_.reset(new ScdTypeWriter(path));
        }
    }

    //void SetMEnd(const MEndFunction& f)
    //{
        //mend_ = f;
    //}

    void Run()
    {
        namespace bfs = boost::filesystem;
        LOG(INFO)<<"Start merging... "<<std::endl;
        std::vector<std::string> scd_list;
        ScdParser::getScdList(scd_path_, scd_list);
        if(scd_list.empty())
        {
            LOG(WARNING)<<"scd list empty"<<std::endl;
            return;
        }
        if(m_==0)
        {
            GenM_();
        }
        cache_.clear();
        for(uint32_t m=0;m<m_;m++)
        {
            LOG(INFO)<<"Processing M "<<m<<std::endl;
            for(uint32_t s=0;s<scd_list.size();s++)
            {
                std::string scd_file = scd_list[s];
                ScdParser parser(izenelib::util::UString::UTF_8);
                LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                SCD_TYPE type = ScdParser::checkSCDType(scd_file);
                parser.load(scd_file);
                uint64_t p=0;
                for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
                {
                    ++p;
                    if(p%100000==0)
                    {
                        LOG(INFO)<<"Processing "<<(std::size_t)p<<" scd docs"<<std::endl; 
                    }
                    Document doc;
                    SCDDoc& scddoc = *(*doc_iter);
                    SCDDoc::iterator p = scddoc.begin();
                    for(; p!=scddoc.end(); ++p)
                    {
                        const std::string& property_name = p->first;
                        doc.property(property_name) = p->second;
                    }
                    if(!ValidM_(doc,m)) continue;
                    ValueType value(doc, type);
                    Process(value);
                }
            }
            MEnd();
        }
        if(writer_) writer_->Close();
        
    }

    void Process(ValueType& value)
    {
        uint128_t id = GetDocId_(value.doc);
        CacheIterator it = cache_.find(id);
        if(it==cache_.end())
        {
            ValueType v;
            Merge(v,value);
            cache_.insert(std::make_pair(id, v));
        }
        else
        {
            Merge(it->second, value);
        }
    }

    void MEnd()
    {
        for(CacheIterator it = cache_.begin();it!=cache_.end();it++)
        {
            ValueType& value = it->second;
            Output(value);
        }
        cache_.clear();
        //if(mend_)
        //{
            //mend_();
        //}
    }

    void Output(ValueType& value)
    {
        if(writer_)
        {
            SCD_TYPE scd_type = value.type;
            if(!all_type_)
            {
                if(scd_type==DELETE_SCD)
                {
                    scd_type = NOT_SCD;
                }
                else if(scd_type==UPDATE_SCD||scd_type==RTYPE_SCD)
                {
                    scd_type = INSERT_SCD;
                }
            }
            if(scd_type == NOT_SCD) return;
            //LOG(INFO)<<"type "<<scd_type<<std::endl;
            
            writer_->Append(value.doc, scd_type);
        }
    }
    
    static void Merge(ValueType& value, const ValueType& another_value)
    {
        if(another_value.type==INSERT_SCD)
        {
            value.doc = another_value.doc;
        }
        else
        {
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
        }
        value.type = ScdTypeMerge(value.type, another_value.type);
    }

    static SCD_TYPE ScdTypeMerge(SCD_TYPE type, SCD_TYPE another_type)
    {
        if(another_type==NOT_SCD) return NOT_SCD;
        else if(type==NOT_SCD) return another_type;
        else if(another_type==DELETE_SCD) return DELETE_SCD;
        else if(another_type==UPDATE_SCD)
        {
            if(type==INSERT_SCD) return INSERT_SCD;
            else return UPDATE_SCD;
        }
        else if(another_type==RTYPE_SCD)
        {
            if(type==INSERT_SCD||type==UPDATE_SCD) return type;
            return RTYPE_SCD;
        }
        else //another==INSERT_SCD
        {
            return INSERT_SCD;
        }
    }

    
    //static void DefaultOutputFunction(Document& doc, int& type, bool i_only)
    //{
        ////LOG(INFO)<<"outputing type: "<<type<<std::endl;
        //if(type==UPDATE_SCD)
        //{
            //if(i_only)
            //{
                //type = INSERT_SCD;
            //}
        //}
        //else if(type==DELETE_SCD)
        //{
            //if(i_only)
            //{
                //type = NOT_SCD;
            //}
            //else
            //{
                //doc.clearExceptDOCID();
            //}
        //}
    //}

    //static OutputFunctionType GetDefaultOutputFunction(bool i_only)
    //{
        //return boost::bind(&DefaultOutputFunction, _1, _2, i_only);
    //}

private:

    void GenM_()
    {
        uint64_t count = ScdParser::getScdDocCount(scd_path_);
        m_ = count/1200000+1;
        LOG(INFO)<<"m set to "<<m_<<std::endl;
    }

    static uint128_t GetDocId_(const Document& doc, const std::string& pname="DOCID")
    {
        std::string sdocid;
        doc.getString(pname, sdocid);
        if(sdocid.empty()) return 0;
        return Utilities::md5ToUint128(sdocid);
        //return B5MHelper::StringToUint128(sdocid);
    }

    bool ValidM_(const Document& doc, uint32_t m) const
    {
        uint128_t id = GetDocId_(doc,mproperty_);
        if(id%m_!=m) return false;
        return true;
    }

    //IdType GenId_(const izenelib::util::UString& id_str)
    //{
        //return Utilities::md5ToUint128(id_str);
    //}

    
private:
    uint32_t m_;
    std::string scd_path_;
    boost::shared_ptr<ScdTypeWriter> writer_;
    //Outputer outputer_;
    //MEndFunction mend_;
    CacheType cache_;
    std::string mproperty_;
    bool all_type_;
    
};

}

#endif


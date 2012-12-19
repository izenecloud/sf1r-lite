#ifndef SF1R_COMMON_PAIRWISESCDMERGER_H_
#define SF1R_COMMON_PAIRWISESCDMERGER_H_
#include <types.h>
#include <algorithm>
#include <common/ScdParser.h>
#include <common/ScdTypeWriter.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

//#define MERGER_DEBUG

namespace sf1r
{

class PairwiseScdMerger
{
public:
    
    struct ValueType
    {
        ValueType():type(NOT_SCD)
        {
        }

        ValueType(const Document& p1, int p2)
        :doc(p1), type(p2)
        {
        }

        Document doc;
        int type;

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
    enum {OLD = 1, EXIST, REPLACE, NEW};
    typedef boost::function<void (ValueType&, const ValueType&) > MergeFunctionType;
    typedef boost::function<void (ValueType&) > Processor;
    typedef boost::function<void (ValueType&,int) > Outputer;
    typedef boost::function<void ()> MEndFunction;

    typedef uint128_t IdType;
    typedef uint32_t PositionType;

    typedef boost::unordered_map<IdType, ValueType> CacheType;
    typedef CacheType::iterator CacheIterator;
    //typedef izenelib::am::succinct::fujimap::Fujimap<IdType, PositionType> PositionMap;





    

    
public:

    PairwiseScdMerger(const std::string& scd_path): m_(0), scd_path_(scd_path), mproperty_("DOCID")
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

    void SetExistScdPath(const std::string& path)
    {
        e_scd_path_ = path;
    }

    void SetOutputPath(const std::string& path)
    {
        if(writer_) writer_.reset();
        if(!path.empty())
        {
            writer_.reset(new ScdTypeWriter(path));
        }
    }

    void SetPreprocessor(const Processor& p)
    {
        pre_processor_ = p;
    }

    void SetOutputer(const Outputer& p)
    {
        outputer_ = p;
    }

    void SetMEnd(const MEndFunction& f)
    {
        mend_ = f;
    }

    void Run()
    {
        namespace bfs = boost::filesystem;
        LOG(INFO)<<"Start merging... "<<std::endl;
        std::vector<std::string> scd_list;
        B5MHelper::GetScdList(scd_path_, scd_list);
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
        std::vector<std::string> e_scd_list;
        B5MHelper::GetScdList(e_scd_path_, e_scd_list);
        for(uint32_t m=0;m<m_;m++)
        {
            LOG(INFO)<<"Processing M "<<m<<std::endl;
            if(!e_scd_list.empty())//if has exist scd, do pre-processing
            {
                for(uint32_t s=0;s<scd_list.size();s++)
                {
                    std::string scd_file = scd_list[s];
                    ScdParser parser(izenelib::util::UString::UTF_8);
                    LOG(INFO)<<"pre-processing "<<scd_file<<std::endl;
                    int type = ScdParser::checkSCDType(scd_file);
                    parser.load(scd_file);
                    uint64_t p=0;
                    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
                    {
                        ++p;
                        if(p%100000==0)
                        {
                            LOG(INFO)<<"pre-processing "<<(std::size_t)p<<" scd docs"<<std::endl; 
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
                        Preprocess(value);
                    }
                }
            }
            for(uint32_t s=0;s<e_scd_list.size();s++)
            {
                std::string scd_file = e_scd_list[s];
                ScdParser parser(izenelib::util::UString::UTF_8);
                LOG(INFO)<<"Processing E "<<scd_file<<std::endl;
                int type = ScdParser::checkSCDType(scd_file);
                parser.load(scd_file);
                uint64_t p=0;
                for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
                {
                    ++p;
                    if(p%100000==0)
                    {
                        LOG(INFO)<<"Processing E "<<(std::size_t)p<<" scd docs"<<std::endl; 
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
                    ProcessE(value);
                }
            }
            for(uint32_t s=0;s<scd_list.size();s++)
            {
                std::string scd_file = scd_list[s];
                ScdParser parser(izenelib::util::UString::UTF_8);
                LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                int type = ScdParser::checkSCDType(scd_file);
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

    void Preprocess(ValueType& value)
    {
        if(pre_processor_)
        {
            pre_processor_(value);
        }
        uint128_t id = GetDocId_(value.doc);
        cache_[id] = ValueType();
        
    }

    void ProcessE(ValueType& value)
    {
        uint128_t id = GetDocId_(value.doc);
        CacheIterator it = cache_.find(id);
        if(it==cache_.end())
        {
            Output(value, EXIST);
        }
        else
        {
            Output(value, OLD);
            DefaultMerge(it->second, value);
        }
    }

    void Process(ValueType& value)
    {
        uint128_t id = GetDocId_(value.doc);
        CacheIterator it = cache_.find(id);
        if(it==cache_.end())
        {
            Output(value, NEW);
        }
        else
        {
            DefaultMerge(it->second, value);
        }
    }

    void MEnd()
    {
        for(CacheIterator it = cache_.begin();it!=cache_.end();it++)
        {
            ValueType& value = it->second;
            Output(value, REPLACE);
        }
        cache_.clear();
        if(mend_)
        {
            mend_();
        }
    }

    void Output(ValueType& value, int status)
    {
        if(outputer_)
        {
            outputer_(value, status);
        }
        if(status!=OLD && writer_)
        {
            writer_->Append(value.doc, value.type);
        }
    }
    
    static void DefaultMerge(ValueType& value, const ValueType& another_value)
    {
        if(another_value.type==DELETE_SCD)
        {
            //doc.clear();
            //doc.property("DOCID") = value.doc.property("DOCID");
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
            value.type = another_value.type;
        }
        else if(another_value.type==UPDATE_SCD)
        {
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
            if(value.type!=INSERT_SCD)
            {
                value.type = another_value.type;
            }
        }
        else if(another_value.type==INSERT_SCD)
        {
            value.doc = another_value.doc;
            value.type = another_value.type;
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
    }

    static uint128_t GetDocId_(const Document& doc, const std::string& pname="DOCID")
    {
        std::string sdocid;
        doc.getString(pname, sdocid);
        if(sdocid.empty()) return 0;
        return B5MHelper::StringToUint128(sdocid);
    }

    bool ValidM_(const Document& doc, uint32_t m) const
    {
        uint128_t id = GetDocId_(doc,mproperty_);
        if(id%m_!=m) return false;
        return true;
    }

    IdType GenId_(const izenelib::util::UString& id_str)
    {
        return Utilities::md5ToUint128(id_str);
        //return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(id_str);
    }

    
private:
    uint32_t m_;
    std::string scd_path_;
    std::string e_scd_path_;
    boost::shared_ptr<ScdTypeWriter> writer_;
    Processor pre_processor_;
    Outputer outputer_;
    MEndFunction mend_;
    CacheType cache_;
    std::string mproperty_;
    
};

}

#endif



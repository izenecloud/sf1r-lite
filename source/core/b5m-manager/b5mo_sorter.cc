#include "b5mo_sorter.h"
#include "product_db.h"
#include "b5m_threadpool.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace sf1r::b5m;
B5moSorter::B5moSorter(const std::string& m, uint32_t mcount)
:m_(m), mcount_(mcount), index_(0), last_pitemid_(1), sort_thread_(NULL)
{
}

void B5moSorter::Append(const ScdDocument& doc, const std::string& ts, int flag)
{
    boost::unique_lock<MutexType> lock(mutex_);
    if(buffer_.size()==mcount_)
    {
        WaitUntilSortFinish_();
        DoSort_();
    }
    buffer_.push_back(Value(doc, ts, flag));
}

bool B5moSorter::StageOne()
{
    boost::unique_lock<MutexType> lock(mutex_);
    WaitUntilSortFinish_();
    if(!buffer_.empty())
    {
        DoSort_();
        WaitUntilSortFinish_();
    }
    return true;
}
bool B5moSorter::StageTwo(bool spu_only, const std::string& last_m, int thread_num)
{
    spu_only_ = spu_only;
    namespace bfs=boost::filesystem;    
    std::string sorter_path = B5MHelper::GetB5moBlockPath(m_); 
    ts_ = bfs::path(m_).filename().string();
    if(ts_==".")
    {
        ts_ = bfs::path(m_).parent_path().filename().string();
    }
    std::string buffer_size = buffer_size_;
    if(buffer_size.empty()) buffer_size = "10G";
    std::string cmd = "sort -m --buffer-size="+buffer_size+" "+sorter_path+"/*";
    if(!last_m.empty())
    {
        std::string last_mirror = B5MHelper::GetB5moMirrorPath(last_m); 
        std::string last_mirror_block = last_mirror+"/block";
        if(!bfs::exists(last_mirror_block))
        {
            if(!GenMirrorBlock_(last_mirror))
            {
                LOG(ERROR)<<"gen mirror block fail"<<std::endl;
                return false;
            }
            if(!GenMBlock_())
            {
                LOG(ERROR)<<"gen b5mo block fail"<<std::endl;
                return false;
            }
        }

        cmd+=" "+last_mirror_block;
    }
    LOG(INFO)<<cmd<<std::endl;
    std::string mirror_path = B5MHelper::GetB5moMirrorPath(m_);
    B5MHelper::PrepareEmptyDir(mirror_path);
    std::string b5mp_path = B5MHelper::GetB5mpPath(m_);
    B5MHelper::PrepareEmptyDir(b5mp_path);
    pwriter_.reset(new ScdTypeWriter(b5mp_path));
    std::string mirror_file = mirror_path+"/block";
    mirror_ofs_.open(mirror_file.c_str());
    FILE* pipe = popen(cmd.c_str(), "r");
    if(!pipe) 
    {
        std::cerr<<"pipe error"<<std::endl;
        return false;
    }
    typedef boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> boost_stream;
    
    int fd = fileno(pipe);
    boost::iostreams::file_descriptor_source d(fd, boost::iostreams::close_handle);
    boost_stream stream(d);
    //stream.set_auto_close(false);
    std::istream is(&stream);
    std::string line;
    std::string spid;
    //typedef std::vector<Value> BufferType;
    typedef PItem BufferType;
    uint32_t id = 1;
    BufferType* buffer = new BufferType;
    buffer->id = id++;
    //thread_num = 1;
    B5mThreadPool<BufferType> pool(thread_num, boost::bind(&B5moSorter::OBag_, this, _1));
    while(std::getline(is, line))
    {
        //if(id%100000==0)
        //{
        //    LOG(INFO)<<"Processing obag id "<<id<<std::endl;
        //}
        Value value;
        if(!value.Parse(line, &json_reader_))
        {
            std::cerr<<"invalid line "<<line<<std::endl;
            continue;
        }
        //std::cerr<<"find value "<<value.spid<<","<<value.is_update<<","<<value.ts<<","<<value.doc.getPropertySize()<<std::endl;

        if(!buffer->odocs.empty())
        {
            if(value.spid!=buffer->odocs.back().spid)
            {
                pool.schedule(buffer);
                buffer = new BufferType;
                buffer->id = id++;
            }
        }
        buffer->odocs.push_back(value);
    }
    pool.schedule(buffer);
    pool.wait();
    mirror_ofs_.close();
    pwriter_->Close();
    pclose(pipe);
    return true;
}

void B5moSorter::WaitUntilSortFinish_()
{
    if(sort_thread_==NULL)
    {
        return;
    }
    sort_thread_->join();
    delete sort_thread_;
    sort_thread_=NULL;
}
void B5moSorter::DoSort_()
{
    std::vector<Value> docs;
    buffer_.swap(docs);
    sort_thread_ = new boost::thread(boost::bind(&B5moSorter::Sort_, this, docs));
}
void B5moSorter::WriteValue_(std::ofstream& ofs, const Value& value)
{
    const ScdDocument& doc = value.doc;
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty()) return;
    Json::Value json_value;
    JsonDocument::ToJson(doc, json_value);
    Json::FastWriter writer;
    std::string str_value = writer.write(json_value);
    boost::algorithm::trim(str_value);
    ofs<<spid<<"\t"<<doc.type<<"\t"<<value.ts<<"\t"<<value.flag<<"\t"<<str_value<<std::endl;
}
void B5moSorter::Sort_(std::vector<Value>& docs)
{
    std::stable_sort(docs.begin(), docs.end(), PidCompare_);
    uint32_t index = ++index_;
    std::string filename = boost::lexical_cast<std::string>(index);
    while(filename.length()<10)
    {
        filename = "0"+filename;
    }
    std::string file = m_+"/"+filename;
    std::ofstream ofs(file.c_str());
    for(uint32_t i=0;i<docs.size();i++)
    {
        //const ScdDocument& doc = docs[i].doc;
        WriteValue_(ofs, docs[i]);
    }
    ofs.close();
}

void B5moSorter::OBag_(PItem& pitem)
{

    bool modified=false;
    for(uint32_t i=0;i<pitem.odocs.size();i++)
    {
        //std::cerr<<"ts "<<docs[i].ts<<","<<ts_<<std::endl;
        if(pitem.odocs[i].ts==ts_)
        {
            modified=true;
            break;
        }
    }
    //std::cerr<<"docs count "<<docs.size()<<","<<modified<<std::endl;
    //if(!modified)
    //{
    //    for(uint32_t i=0;i<docs.size();i++)
    //    {
    //        WriteValueSafe_(mirror_ofs_, docs[i].doc, docs[i].ts);
    //    }
    //    return;
    //}

    if(modified)
    {
        std::vector<Value>& docs = pitem.odocs;
        std::sort(docs.begin(), docs.end(), OCompare_);
        std::vector<ScdDocument> prev_odocs;
        std::vector<ScdDocument> odocs;
        std::vector<Value> ovalues;
        std::vector<Value> pre_ovalues;
        for(uint32_t i=0;i<docs.size();i++)
        {
            const Value& v = docs[i];
            ODocMerge_(ovalues, v);
            if(v.ts<ts_)
            {
                ODocMerge_(pre_ovalues, v);
            }
            //LOG(INFO)<<"doc "<<v.spid<<","<<v.ts<<","<<v.doc.type<<","<<v.doc.property("DOCID")<<std::endl;
            //const ScdDocument& doc = v.doc;
            //ODocMerge_(odocs, doc);
            //if(v.ts<ts_)
            //{
            //    ODocMerge_(prev_odocs, doc);
            //}
        }
        for(uint32_t i=0;i<pre_ovalues.size();i++)
        {
            prev_odocs.push_back(pre_ovalues[i].doc);
        }
        {
            std::size_t j=0;
            for(std::size_t i=0;i<ovalues.size();i++)
            {
                Value& v = ovalues[i];
                //LOG(INFO)<<"ovalue "<<v.spid<<","<<v.doc.property("DOCID")<<","<<(int)v.doc.type<<std::endl;
                v.diff_doc = v.doc;
                if(v.doc.type!=UPDATE_SCD) continue;
                for(;j<prev_odocs.size();j++)
                {
                    ScdDocument& last = v.doc;
                    const ScdDocument& jdoc = prev_odocs[j];
                    std::string last_docid;
                    std::string last_jdocid;
                    last.getString("DOCID", last_docid);
                    jdoc.getString("DOCID", last_jdocid);
                    if(last_docid<last_jdocid)
                    {
                        break;
                    }
                    else if(last_docid==last_jdocid)
                    {
                        v.diff_doc.diff(jdoc);
                        break;
                    }
                }
            }
        }
        pitem.odocs = ovalues;
        for(uint32_t i=0;i<ovalues.size();i++)
        {
            odocs.push_back(ovalues[i].doc);
        }
        //for(uint32_t i=0;i<odocs.size();i++)
        //{
        //    if(odocs[i].type!=DELETE_SCD)
        //    {
        //        Value v;
        //        v.doc = odocs[i];
        //        v.ts = ts_;
        //        pitem.odocs.push_back(v);
        //    }
        //}
        ScdDocument& pdoc = pitem.pdoc;
        pgenerator_.Gen(odocs, pdoc, spu_only_);
        if(pdoc.type==NOT_SCD)
        {
        }
        else if(pdoc.type==DELETE_SCD)
        {
        }
        else
        {
            ScdDocument prev_pdoc;
            if(!prev_odocs.empty())
            {
                pgenerator_.Gen(prev_odocs, prev_pdoc);
                pdoc.diff(prev_pdoc);
            }
            int64_t itemcount=0;
            pdoc.getProperty("itemcount", itemcount);
            //if(itemcount>=100) return;
            SCD_TYPE ptype = pdoc.type;
            if(pdoc.getPropertySize()<2)
            {
                ptype = NOT_SCD;
            }
            else if(pdoc.getPropertySize()==2)//DATE only update
            {
                if(pdoc.hasProperty("DATE"))
                {
                    ptype = NOT_SCD;
                }
            }
            pdoc.type = ptype;
        }
        //if(pdoc.type!=NOT_SCD)
        //{
        //    boost::unique_lock<boost::mutex> lock(mutex_);
        //    pwriter_->Append(pdoc);
        //}
    }
    for(uint32_t i=0;i<pitem.odocs.size();i++)
    {
        const ScdDocument& doc = pitem.odocs[i].doc;
        std::string spid;
        doc.getString("uuid", spid);
        if(spid.empty()) continue;
        if(doc.type!=UPDATE_SCD) continue;
        Json::Value json_value;
        JsonDocument::ToJson(doc, json_value);
        //for(Document::property_const_iterator it=doc.propertyBegin();it!=doc.propertyEnd();++it)
        //{
            //const PropertyValue& v = it->second;
            //const UString& uv = v.get<UString>();
            //std::string sv;
            //uv.convertString(sv, UString::UTF_8);
            //json_value[it->first] = sv;
        //}
        Json::FastWriter writer;
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        std::stringstream ss;
        ss<<spid<<"\t"<<doc.type<<"\t"<<pitem.odocs[i].ts<<"\t"<<pitem.odocs[i].flag<<"\t"<<str_value;
        pitem.odocs[i].text = ss.str();
    }
    WritePItem_(pitem);
    
}

void B5moSorter::WritePItem_(PItem& pitem)
{
    while(true)
    {
        //boost::upgrade_lock<boost::shared_mutex> lock(mutex_);
        if(pitem.id==last_pitemid_)
        {
            //boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
            boost::unique_lock<MutexType> lock(mutex_);
            if(pitem.id%100000==0)
            {
                LOG(INFO)<<"Processing pitem id "<<pitem.id<<std::endl;
            }
            //LOG(INFO)<<"pid "<<pitem.pdoc.property("DOCID")<<","<<pitem.pdoc.type<<std::endl;
            
            //std::cerr<<"write pitem id "<<pitem.id<<","<<last_pitemid_<<std::endl;
            //for(uint32_t i=0;i<pitem.odocs.size();i++)
            //{
            //    WriteValue_(mirror_ofs_, pitem.odocs[i].doc, pitem.odocs[i].ts);
            //}
            for(uint32_t i=0;i<pitem.odocs.size();i++)
            {
                if(!pitem.odocs[i].text.empty())
                {
                    mirror_ofs_<<pitem.odocs[i].text<<std::endl;
                }
            }
            uint32_t odoc_count=0;
            uint32_t odoc_precount=0;
            for(uint32_t i=0;i<pitem.odocs.size();i++)
            {
                Value& value = pitem.odocs[i];
                ScdDocument& odoc = value.diff_doc;
                odoc_precount++;
                if(odoc.type!=UPDATE_SCD) continue;
                odoc_count++;
                if(value.ts<ts_) continue;
                if(odoc.getPropertySize()<2) continue;
                odoc.property("itemcount") = (int64_t)1;
                //odoc.eraseProperty("uuid");
                //LOG(INFO)<<"output OU "<<odoc.property("DOCID")<<std::endl;
                pwriter_->Append(odoc);
            }
            //LOG(INFO)<<"odoc stat "<<odoc_count<<","<<odoc_precount<<std::endl;
            for(uint32_t i=0;i<pitem.odocs.size();i++)
            {
                Value& value = pitem.odocs[i];
                if(value.ts<ts_) continue;
                ScdDocument& odoc = value.doc;
                if(odoc.type==DELETE_SCD&&value.flag==0)
                {
                    odoc.clearExceptDOCID();
                    //LOG(INFO)<<"output OD "<<odoc.property("DOCID")<<std::endl;
                    pwriter_->Append(odoc);
                }
            }
            if(odoc_count>1)
            {
                if(pitem.pdoc.type!=NOT_SCD)
                {
                    //LOG(INFO)<<"output P "<<pitem.pdoc.property("DOCID")<<std::endl;
                    pwriter_->Append(pitem.pdoc);
                }
            }
            else if(odoc_precount>1&&pitem.pdoc.type!=NOT_SCD)
            {
                pitem.pdoc.type = DELETE_SCD;
                pitem.pdoc.clearExceptDOCID();
                //LOG(INFO)<<"output PD "<<pitem.pdoc.property("DOCID")<<std::endl;
                pwriter_->Append(pitem.pdoc);
            }
            last_pitemid_ = pitem.id+1;
            break;
        }
        else
        {
            sched_yield();
        }
    }
}
void B5moSorter::ODocMerge_(std::vector<ScdDocument>& vec, const ScdDocument& doc)
{
    if(vec.empty()) vec.push_back(doc);
    else
    {
        if(vec.back().property("DOCID")==doc.property("DOCID"))
        {
            vec.back().merge(doc);
        }
        else
        {
            vec.push_back(doc);
        }
    }
}
void B5moSorter::ODocMerge_(std::vector<Value>& vec, const Value& doc)
{
    if(vec.empty()) vec.push_back(doc);
    else
    {
        if(vec.back().doc.property("DOCID")==doc.doc.property("DOCID"))
        {
            //vec.back().doc.merge(doc.doc);
            //vec.back().ts = doc.ts;
            vec.back().merge(doc);
        }
        else
        {
            vec.push_back(doc);
        }
    }
}

bool B5moSorter::GenMirrorBlock_(const std::string& mirror_path)
{
    std::vector<std::string> scd_list;
    ScdParser::getScdList(mirror_path, scd_list);
    if(scd_list.size()!=1)
    {
        return false;
    }
    std::string ts = boost::filesystem::path(mirror_path).parent_path().filename().string();
    std::string scd_file = scd_list.front();
    SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    std::ofstream block_ofs((mirror_path+"/block").c_str());
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Mirror Documents "<<n<<std::endl;
        }
        ScdDocument doc;
        doc.type = scd_type;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();
        for(; p!=scddoc.end(); ++p)
        {
            doc.property(p->first) = p->second;
        }
        Value v(doc, ts);
        WriteValue_(block_ofs, v);
    }
    block_ofs.close();


    return true;
}
bool B5moSorter::GenMBlock_()
{
    std::string b5mo_path = B5MHelper::GetB5moPath(m_);
    std::string block_path = B5MHelper::GetB5moBlockPath(m_);
    std::vector<std::string> scd_list;
    ScdParser::getScdList(b5mo_path, scd_list);
    if(scd_list.size()==0)
    {
        return false;
    }
    B5MHelper::PrepareEmptyDir(block_path);
    std::vector<Value> values;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%100000==0)
            {
                LOG(INFO)<<"Find B5mo Documents "<<n<<std::endl;
            }
            Value value;
            value.doc.type = scd_type;
            value.ts = ts_;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                value.doc.property(p->first) = p->second;
            }
            values.push_back(value);
        }
    }
    std::sort(values.begin(), values.end(), PidCompare_);
    std::string block_file = block_path+"/1";
    std::ofstream ofs(block_file.c_str());
    for(uint32_t i=0;i<values.size();i++)
    {
        WriteValue_(ofs, values[i]);
    }
    ofs.close();
    return true;
}


#include "b5mo_sorter.h"
#include "product_db.h"
#include "b5m_threadpool.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace sf1r::b5m;
B5moSorter::B5moSorter(const std::string& m, uint32_t mcount)
:m_(m), mcount_(mcount), index_(0), sort_thread_(NULL)
{
}

void B5moSorter::Append(const ScdDocument& doc, const std::string& ts, int flag)
{
    boost::unique_lock<MutexType> lock(mutex_);
    if(!ofs_.is_open())
    {
        std::string file = m_+"/block";
        ofs_.open(file.c_str());
    }
    Value v(doc, ts, flag);
    WriteValue_(ofs_, v);
    //if(buffer_.size()==mcount_)
    //{
    //    WaitUntilSortFinish_();
    //    DoSort_();
    //}
    //buffer_.push_back(Value(doc, ts, flag));
}

bool B5moSorter::StageOne()
{
    boost::unique_lock<MutexType> lock(mutex_);
    ofs_.close();
    //WaitUntilSortFinish_();
    //if(!buffer_.empty())
    //{
    //    DoSort_();
    //    WaitUntilSortFinish_();
    //}
    return true;
}
bool B5moSorter::StageTwo(const std::string& last_m, int thread_num)
{
    namespace bfs=boost::filesystem;    
    std::string tmp_path = B5MHelper::GetTmpPath(m_);
    B5MHelper::PrepareEmptyDir(tmp_path);
    std::string sorter_path = B5MHelper::GetB5moBlockPath(m_); 
    ts_ = bfs::path(m_).filename().string();
    if(ts_==".")
    {
        ts_ = bfs::path(m_).parent_path().filename().string();
    }
    std::string buffer_size = buffer_size_;
    if(buffer_size.empty()) buffer_size = "30%";
    std::string sort_bin = sorter_bin_;
    if(sort_bin.empty()) sort_bin = "sort";
    LOG(INFO)<<"sorter bin : "<<sort_bin<<std::endl;
    std::string cmd = sort_bin+" --stable --field-separator='\t' -k1,1 --buffer-size="+buffer_size+" -T "+tmp_path+" "+sorter_path+"/block > "+sorter_path+"/block.sort";
    LOG(INFO)<<"cmd : "<<cmd<<std::endl;
    int status = system(cmd.c_str());
    LOG(INFO)<<"cmd finished : "<<status<<std::endl;
    std::string last_mirror_file;
    if(!last_m.empty())
    {
        std::string last_mirror = B5MHelper::GetB5moMirrorPath(last_m); 
        last_mirror_file = last_mirror+"/block";
        if(!bfs::exists(last_mirror_file))
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
    }
    std::string mirror_path = B5MHelper::GetB5moMirrorPath(m_);
    B5MHelper::PrepareEmptyDir(mirror_path);
    //std::string b5mp_path = B5MHelper::GetB5mpPath(m_);
    std::string b5mp_path = b5mm_.b5mp_path;
    B5MHelper::PrepareEmptyDir(b5mp_path);
    pwriter_.reset(new ScdTypeWriter(b5mp_path));
    std::string mirror_file = mirror_path+"/block";
    std::string block_file = sorter_path+"/block.sort";
    ordered_writer_.reset(new OrderedWriter(mirror_file));
    //mirror_ofs_.open(mirror_file.c_str());
    typedef PItem BufferType;
    std::string spid;
    uint32_t id = 1;
    BufferType* buffer = new BufferType;
    buffer->id = id++;
    B5mThreadPool<BufferType> pool(thread_num, boost::bind(&B5moSorter::OBag_, this, _1));

    std::ifstream is1(block_file.c_str());
    std::ifstream is2;
    std::string line1;
    std::string line2;
    if(!last_mirror_file.empty()) 
    {
        is2.open(last_mirror_file.c_str());
    }
    while(true)
    {
        if(line1.empty()&&is1.good()) std::getline(is1, line1);
        if(line2.empty()&&is2.good()) std::getline(is2, line2);
        std::string line;
        if(line1.empty())
        {
            if(!line2.empty()) 
            {
                line = line2;
                std::getline(is2, line2);
            }
        }
        else 
        {
            if(line2.empty()) 
            {
                line = line1;
                std::getline(is1, line1);
            }
            else
            {
                if(line1<line2)
                {
                    line = line1;
                    std::getline(is1, line1);
                }
                else
                {
                    line = line2;
                    std::getline(is2, line2);
                }
            }
        }
        if(line.empty()) break;

        Value value;
        if(!value.ParsePid(line))
        {
            LOG(WARNING)<<"invalid line "<<line<<std::endl;
            continue;
        }

        if(!buffer->odocs.empty())
        {
            if(value.spid!=buffer->odocs.back().spid)
            {
                pool.schedule(buffer);
                buffer = new BufferType;
                buffer->id = id++;
                if(buffer->id%100000==0)
                {
                    LOG(INFO)<<"Processing pid "<<buffer->id<<std::endl;
                }
            }
        }
        buffer->odocs.push_back(value);
    }
    is1.close();
    if(is2.is_open()) is2.close();


    //FILE* pipe = popen(cmd.c_str(), "r");
    //if(!pipe) 
    //{
    //    std::cerr<<"pipe error"<<std::endl;
    //    return false;
    //}
    //typedef boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> boost_stream;
    //
    //int fd = fileno(pipe);
    //boost::iostreams::file_descriptor_source d(fd, boost::iostreams::close_handle);
    //boost_stream stream(d);
    ////stream.set_auto_close(false);
    //std::istream is(&stream);
    //while(std::getline(is, line))
    //{
    //    //if(id%100000==0)
    //    //{
    //    //    LOG(INFO)<<"Processing obag id "<<id<<std::endl;
    //    //}
    //    Value value;
    //    if(!value.Parse(line, &json_reader_))
    //    {
    //        std::cerr<<"invalid line "<<line<<std::endl;
    //        continue;
    //    }
    //    //std::cerr<<"find value "<<value.spid<<","<<value.is_update<<","<<value.ts<<","<<value.doc.getPropertySize()<<std::endl;

    //    if(!buffer->odocs.empty())
    //    {
    //        if(value.spid!=buffer->odocs.back().spid)
    //        {
    //            pool.schedule(buffer);
    //            buffer = new BufferType;
    //            buffer->id = id++;
    //        }
    //    }
    //    buffer->odocs.push_back(value);
    //}
    //pclose(pipe);
    pool.schedule(buffer);
    pool.wait();
    ordered_writer_->Finish();
    //mirror_ofs_.close();
    pwriter_->Close();
    boost::filesystem::remove_all(tmp_path);
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

    Json::Reader json_reader;
    for(uint32_t i=0;i<pitem.odocs.size();i++)
    {
        pitem.odocs[i].Parse(pitem.odocs[i].text, &json_reader);
        pitem.odocs[i].text.clear();
    }
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

    if(modified)
    {
        std::vector<Value>& docs = pitem.odocs;
        std::sort(docs.begin(), docs.end(), OCompare_);
        std::vector<Value> ovalues;
        std::vector<Value> prev_ovalues;
        for(uint32_t i=0;i<docs.size();i++)
        {
            const Value& v = docs[i];
            ODocMerge_(ovalues, v);
            if(v.ts<ts_)
            {
                ODocMerge_(prev_ovalues, v);
            }
        }
        ScdDocument& pdoc = pitem.pdoc;
        ScdDocument prev_pdoc;
        {
            std::vector<ScdDocument> odocs;
            std::vector<ScdDocument> prev_odocs;
            for(uint32_t i=0;i<ovalues.size();i++)
            {
                odocs.push_back(ovalues[i].doc);
            }
            for(uint32_t i=0;i<prev_ovalues.size();i++)
            {
                prev_odocs.push_back(prev_ovalues[i].doc);
            }
            pgenerator_.Gen(odocs, pdoc);
            if(!prev_odocs.empty())
            {
                pgenerator_.Gen(prev_odocs, prev_pdoc);
            }
        }
        SetAttributes_(ovalues, pdoc);
        SetAttributes_(prev_ovalues, prev_pdoc);
        {
            std::size_t j=0;
            for(std::size_t i=0;i<ovalues.size();i++)
            {
                Value& v = ovalues[i];
                //LOG(INFO)<<"ovalue "<<v.spid<<","<<v.doc.property("DOCID")<<","<<(int)v.doc.type<<std::endl;
                v.diff_doc = v.doc;
                if(v.doc.type!=UPDATE_SCD) continue;
                for(;j<prev_ovalues.size();j++)
                {
                    ScdDocument& last = v.doc;
                    const ScdDocument& jdoc = prev_ovalues[j].doc;
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
        if(pdoc.type==NOT_SCD)
        {
        }
        else if(pdoc.type==DELETE_SCD)
        {
        }
        else
        {
            int64_t prev_itemcount=0;
            prev_pdoc.getProperty("itemcount", prev_itemcount);
            if(prev_itemcount>1)
            {
                pdoc.diff(prev_pdoc);
            }
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
        Json::FastWriter writer;
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        std::stringstream ss;
        ss<<spid<<"\t"<<doc.type<<"\t"<<pitem.odocs[i].ts<<"\t"<<pitem.odocs[i].flag<<"\t"<<str_value;
        pitem.odocs[i].text = ss.str();
    }
    WritePItem_(pitem);
    
}
void B5moSorter::SetAttributes_(std::vector<Value>& values, const ScdDocument& pdoc)
{
    Document::str_type attrib;
    pdoc.getProperty("Attribute", attrib);
    if(!attrib.empty())
    {
        for(std::size_t i=0;i<values.size();i++)
        {
            ScdDocument& doc = values[i].doc;
            doc.property("Attribute") = attrib;
        }
    }
}

//void B5moSorter::WritePItem_(PItem& pitem)
//{
//    while(true)
//    {
//        //boost::upgrade_lock<boost::shared_mutex> lock(mutex_);
//        if(pitem.id==last_pitemid_)
//        {
//            //boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
//            boost::unique_lock<MutexType> lock(mutex_);
//            if(pitem.id%100000==0)
//            {
//                LOG(INFO)<<"Processing pitem id "<<pitem.id<<std::endl;
//            }
//            //LOG(INFO)<<"pid "<<pitem.pdoc.property("DOCID")<<","<<pitem.pdoc.type<<std::endl;
//            
//            //std::cerr<<"write pitem id "<<pitem.id<<","<<last_pitemid_<<std::endl;
//            //for(uint32_t i=0;i<pitem.odocs.size();i++)
//            //{
//            //    WriteValue_(mirror_ofs_, pitem.odocs[i].doc, pitem.odocs[i].ts);
//            //}
//            for(uint32_t i=0;i<pitem.odocs.size();i++)
//            {
//                if(!pitem.odocs[i].text.empty())
//                {
//                    ordered_writer_->Insert(pitem.id, pitem.odocs[i].text);
//                    //mirror_ofs_<<pitem.odocs[i].text<<std::endl;
//                }
//            }
//            uint32_t odoc_count=0;
//            uint32_t odoc_precount=0;
//            for(uint32_t i=0;i<pitem.odocs.size();i++)
//            {
//                Value& value = pitem.odocs[i];
//                ScdDocument& odoc = value.diff_doc;
//                odoc_precount++;
//                if(odoc.type!=UPDATE_SCD) continue;
//                odoc_count++;
//                if(value.ts<ts_) continue;
//                if(odoc.getPropertySize()<2) continue;
//                odoc.property("itemcount") = (int64_t)1;
//                //odoc.eraseProperty("uuid");
//                //LOG(INFO)<<"output OU "<<odoc.property("DOCID")<<std::endl;
//                pwriter_->Append(odoc);
//            }
//            //LOG(INFO)<<"odoc stat "<<odoc_count<<","<<odoc_precount<<std::endl;
//            for(uint32_t i=0;i<pitem.odocs.size();i++)
//            {
//                Value& value = pitem.odocs[i];
//                if(value.ts<ts_) continue;
//                ScdDocument& odoc = value.doc;
//                if(odoc.type==DELETE_SCD&&value.flag==0)
//                {
//                    odoc.clearExceptDOCID();
//                    //LOG(INFO)<<"output OD "<<odoc.property("DOCID")<<std::endl;
//                    pwriter_->Append(odoc);
//                }
//            }
//            if(odoc_count>1)
//            {
//                if(pitem.pdoc.type!=NOT_SCD)
//                {
//                    //LOG(INFO)<<"output P "<<pitem.pdoc.property("DOCID")<<std::endl;
//                    pwriter_->Append(pitem.pdoc);
//                }
//            }
//            else if(odoc_precount>1&&pitem.pdoc.type!=NOT_SCD)
//            {
//                pitem.pdoc.type = DELETE_SCD;
//                pitem.pdoc.clearExceptDOCID();
//                //LOG(INFO)<<"output PD "<<pitem.pdoc.property("DOCID")<<std::endl;
//                pwriter_->Append(pitem.pdoc);
//            }
//            last_pitemid_ = pitem.id+1;
//            break;
//        }
//        else
//        {
//            sched_yield();
//        }
//    }
//}
void B5moSorter::WritePItem_(PItem& pitem)
{
    std::vector<std::string> mtexts;
    for(uint32_t i=0;i<pitem.odocs.size();i++)
    {
        if(!pitem.odocs[i].text.empty())
        {
            mtexts.push_back(pitem.odocs[i].text);
            //mirror_ofs_<<pitem.odocs[i].text<<std::endl;
        }
    }
    ordered_writer_->Insert(pitem.id, mtexts);
    typedef std::pair<std::string, SCD_TYPE> WriteDoc;
    std::vector<WriteDoc> write_docs;
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
        std::string doctext;
        ScdWriter::DocToString(odoc, doctext);
        write_docs.push_back(std::make_pair(doctext, odoc.type));
        //odoc.eraseProperty("uuid");
        //LOG(INFO)<<"output OU "<<odoc.property("DOCID")<<std::endl;
        //boost::unique_lock<MutexType> lock(mutex_);
        //pwriter_->Append(odoc);
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
            std::string doctext;
            ScdWriter::DocToString(odoc, doctext);
            write_docs.push_back(std::make_pair(doctext, odoc.type));
            //LOG(INFO)<<"output OD "<<odoc.property("DOCID")<<std::endl;
            //boost::unique_lock<MutexType> lock(mutex_);
            //pwriter_->Append(odoc);
        }
    }
    if(odoc_count>1)
    {
        if(pitem.pdoc.type!=NOT_SCD)
        {
            std::string doctext;
            ScdWriter::DocToString(pitem.pdoc, doctext);
            write_docs.push_back(std::make_pair(doctext, pitem.pdoc.type));
            //LOG(INFO)<<"output P "<<pitem.pdoc.property("DOCID")<<std::endl;
            //boost::unique_lock<MutexType> lock(mutex_);
            //pwriter_->Append(pitem.pdoc);
        }
    }
    else if(odoc_precount>1&&pitem.pdoc.type!=NOT_SCD)
    {
        pitem.pdoc.type = DELETE_SCD;
        pitem.pdoc.clearExceptDOCID();
        std::string doctext;
        ScdWriter::DocToString(pitem.pdoc, doctext);
        write_docs.push_back(std::make_pair(doctext, pitem.pdoc.type));
        //LOG(INFO)<<"output PD "<<pitem.pdoc.property("DOCID")<<std::endl;
        //boost::unique_lock<MutexType> lock(mutex_);
        //pwriter_->Append(pitem.pdoc);
    }
    {
        boost::unique_lock<MutexType> lock(mutex_);
        for(uint32_t i=0;i<write_docs.size();i++)
        {
            pwriter_->Append(write_docs[i].first, write_docs[i].second);
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


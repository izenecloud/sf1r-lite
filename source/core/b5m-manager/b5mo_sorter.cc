#include "b5mo_sorter.h"
#include "product_db.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace sf1r;
B5moSorter::B5moSorter(const std::string& m, uint32_t mcount)
:m_(m), mcount_(mcount), index_(0), sort_thread_(NULL)
{
}

void B5moSorter::Append(const ScdDocument& doc, const std::string& ts)
{
    if(buffer_.size()==mcount_)
    {
        WaitUntilSortFinish_();
        DoSort_();
    }
    buffer_.push_back(Value(doc, ts));
}

bool B5moSorter::StageOne()
{
    WaitUntilSortFinish_();
    if(!buffer_.empty())
    {
        DoSort_();
        WaitUntilSortFinish_();
    }
    return true;
}
bool B5moSorter::StageTwo(const std::string& last_m)
{
    namespace bfs=boost::filesystem;    
    std::string sorter_path = B5MHelper::GetB5moBlockPath(m_); 
    ts_ = bfs::path(m_).filename().string();
    if(ts_==".")
    {
        ts_ = bfs::path(m_).parent_path().filename().string();
    }
    std::string cmd = "sort -m --buffer-size=1000M "+sorter_path+"/*";
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
    std::vector<Value> buffer;
    while(std::getline(is, line))
    {
        Value value;
        if(!value.Parse(line, &json_reader_))
        {
            std::cerr<<"invalid line "<<line<<std::endl;
            continue;
        }
        //std::cerr<<"find value "<<value.spid<<","<<value.is_update<<","<<value.ts<<","<<value.doc.getPropertySize()<<std::endl;

        if(buffer.empty()) buffer.push_back(value);
        if(!buffer.empty())
        {
            if(value.spid!=buffer.back().spid)
            {
                OBag_(buffer);
                buffer.resize(0);
            }
        }
        buffer.push_back(value);
    }
    OBag_(buffer);
    mirror_ofs_.close();
    pwriter_->Close();
    //char buffer[128];
    //std::string result = "";
    //while(!feof(pipe))
    //{
        //if(fgets(buffer, 128, pipe)!=NULL)
        //{
            //result += buffer;
        //}
    //}
    pclose(pipe);
    //boost::algorithm::trim(result);
    //std::size_t c = boost::lexical_cast<std::size_t>(result);
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
void B5moSorter::WriteValue_(std::ofstream& ofs, const ScdDocument& doc, const std::string& ts)
{
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty()) return;
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
    ofs<<spid<<"\t"<<doc.type<<"\t"<<ts<<"\t"<<str_value<<std::endl;
}
void B5moSorter::Sort_(std::vector<Value>& docs)
{
    std::sort(docs.begin(), docs.end(), PidCompare_);
    uint32_t index = ++index_;
    std::string file = m_+"/"+boost::lexical_cast<std::string>(index);
    std::ofstream ofs(file.c_str());
    for(uint32_t i=0;i<docs.size();i++)
    {
        const ScdDocument& doc = docs[i].doc;
        WriteValue_(ofs, doc, docs[i].ts);
    }
    ofs.close();
}

void B5moSorter::OBag_(std::vector<Value>& docs)
{
    bool modified=false;
    for(uint32_t i=0;i<docs.size();i++)
    {
        //std::cerr<<"ts "<<docs[i].ts<<","<<ts_<<std::endl;
        if(docs[i].ts==ts_)
        {
            modified=true;
            break;
        }
    }
    //std::cerr<<"docs count "<<docs.size()<<","<<modified<<std::endl;
    if(!modified)
    {
        for(uint32_t i=0;i<docs.size();i++)
        {
            WriteValue_(mirror_ofs_, docs[i].doc, docs[i].ts);
        }
        return;
    }

    std::sort(docs.begin(), docs.end(), OCompare_);
    std::vector<ScdDocument> prev_odocs;
    std::vector<ScdDocument> odocs;
    //Document pdoc;
    //ProductProperty pp;
    //Document prev_odoc;
    //Document odoc;
    //std::string last_soid;
    for(uint32_t i=0;i<docs.size();i++)
    {
        const Value& v = docs[i];
        const ScdDocument& doc = v.doc;
        ODocMerge_(odocs, doc);
        if(v.ts<ts_)
        {
            ODocMerge_(prev_odocs, doc);
        }
    }
    //std::cerr<<"odocs count "<<odocs.size()<<std::endl;
    for(uint32_t i=0;i<odocs.size();i++)
    {
        if(odocs[i].type!=DELETE_SCD)
        {
            WriteValue_(mirror_ofs_, odocs[i], ts_);
        }
    }
    ScdDocument pdoc;
    pgenerator_.Gen(odocs, pdoc);
    if(pdoc.type==DELETE_SCD)
    {
        pwriter_->Append(pdoc, pdoc.type);
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
        if(itemcount>=100) return;
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
        if(ptype!=NOT_SCD)
        {
            pwriter_->Append(pdoc, ptype);
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
        WriteValue_(block_ofs, doc, ts);
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
        WriteValue_(ofs, values[i].doc, values[i].ts);
    }
    ofs.close();
    return true;
}


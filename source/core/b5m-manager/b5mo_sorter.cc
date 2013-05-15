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
    }
    return true;
}
bool B5moSorter::StageTwo(const std::string& last_m)
{
    namespace bfs=boost::filesystem;    
    std::string sorter_path = B5MHelper::GetB5moBlockPath(m_); 
    ts_ = bfs::path(m_).filename().string();
    std::string cmd = "sort -m --buffer-size=1000M "+sorter_path+"/*";
    if(!last_m.empty())
    {
        std::string last_sorter_path = B5MHelper::GetB5moMirrorPath(last_m); 
        cmd+=" "+last_sorter_path+"/block";
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
        if(!value.Parse(line))
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
        if(docs[i].ts==ts_)
        {
            modified=true;
            break;
        }
    }
    if(!modified) return;

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
    for(uint32_t i=0;i<odocs.size();i++)
    {
        WriteValue_(mirror_ofs_, odocs[i], ts_);
    }
    ScdDocument pdoc;
    ScdDocument prev_pdoc;
    B5mpDoc::Gen(odocs, pdoc);
    if(!prev_odocs.empty())
    {
        B5mpDoc::Gen(prev_odocs, prev_pdoc);
    }
    Document diff_pdoc;
    pdoc.diff(prev_pdoc, diff_pdoc);
    SCD_TYPE ptype = pdoc.type;
    if(diff_pdoc.getPropertySize()<2)
    {
        ptype = NOT_SCD;
    }
    else if(diff_pdoc.getPropertySize()==2)//DATE only update
    {
        if(diff_pdoc.hasProperty("DATE"))
        {
            ptype = NOT_SCD;
        }
    }
    if(ptype!=NOT_SCD)
    {
        pwriter_->Append(diff_pdoc, ptype);
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

